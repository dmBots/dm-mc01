# MC01-RC CAN/CAN-FD 实板测试报告

## 1. 测试结论

测试日期：2026-09-07

在板卡接入 24 V 电源、CAN1 与 CAN2 物理接口互连的条件下，以下三档预设全部通过：

- Classic CAN：仲裁速率 1 Mbit/s，8 字节数据帧
- CAN-FD 5M：仲裁速率 1 Mbit/s，数据速率 5 Mbit/s，64 字节 BRS 数据帧
- CAN-FD 8M：仲裁速率 1 Mbit/s，数据速率 8 Mbit/s，64 字节 BRS 数据帧

每档预设均完成 CAN1、CAN2 各自的内部回环，以及 CAN1→CAN2、CAN2→CAN1 的 Normal 模式外部回环。共 12 个测试项，12 个通过。

## 2. 测试环境

- 目标器件：STM32C5A3RGT6
- 调试器：DAPLink，UID `D641959C00B43ECC`
- 下载与调试工具：项目 `.venv` 中的 pyOCD
- 构建预设：`debug_GCC_STM32C5A3RGT6`
- 板卡供电：24 V 已接入
- 总线连接：CAN1 与 CAN2 的 CANH、CANL 对应互连
- 最终留在板上的预设：`BOARD_CAN_PRESET_CANFD_8M`

## 3. 接收处理方式

接收路径使用中断，不在主循环中轮询硬件 RX FIFO：

1. FDCAN1/FDCAN2 的 RX FIFO0 新消息中断触发对应 `FDCANx_IT0_IRQHandler`。
2. HAL 中断处理调用 `HAL_FDCAN_RxFifo0Callback()`。
3. BSP 回调在中断上下文中清空硬件 RX FIFO0，并将报文写入每端口独立的软件环形队列。
4. `app/can_test.c` 在主循环中以非阻塞方式轮询软件队列，完成报文内容校验和测试状态机推进。

因此，硬件 FIFO 的及时搬运由中断保证，应用层仍保持轮询式多任务结构。

## 4. 测试方法

每档预设均执行以下步骤：

1. 在 `board/board_config.h` 切换 `BOARD_CAN_PRESET`。
2. 使用 CMake Debug 预设完整编译。
3. 使用 DAPLink + pyOCD 下载 ELF。
4. 复位并运行 1 秒。
5. 通过调试器读取 `test_status`，核对测试结果、通过位图、最后接收 ID、TEC、REC 和软件 RX 队列丢包计数。

通过位图定义：

- bit 0：CAN1 内部回环
- bit 1：CAN2 内部回环
- bit 2：CAN1→CAN2 外部回环
- bit 3：CAN2→CAN1 外部回环

## 5. 测试结果

| 预设 | CAN1 内部回环 | CAN2 内部回环 | CAN1→CAN2 | CAN2→CAN1 | passed_mask | TEC/REC | RX 丢包 | 结果 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Classic 1M | 通过 | 通过 | 通过 | 通过 | `0x0F` | 两路均为 0 | 两路均为 0 | 通过 |
| CAN-FD 5M | 通过 | 通过 | 通过 | 通过 | `0x0F` | 两路均为 0 | 两路均为 0 | 通过 |
| CAN-FD 8M | 通过 | 通过 | 通过 | 通过 | `0x0F` | 两路均为 0 | 两路均为 0 | 通过 |

三次测试的最终状态均为：

- `result = 2`：通过
- `phase = 12`：完成
- `failure_code = 0`
- `last_rx_identifier = 0x202`
- CAN1/CAN2 的发送错误计数、接收错误计数、软件队列丢包计数均为 0

## 6. 补充观测

- `GPIOC_ODR = 0x0000C000`，PC14、PC15 保持高电平，板载相关 5 V 电源使能有效。
- `GPIOB_IDR = 0x00003310`，测试结束后的 PB8（FDCAN1_RX）和 PB12（FDCAN2_RX）均处于隐性高电平。
- 未接入 24 V 时，内部回环仍可通过，但 Normal 模式无应答，发送端 TEC 会升高；接入 24 V 后该现象消失。
- pyOCD 下载时提示 DFP 中缺少 Flash programming debug sequence，已自动回退到 FLM；三次下载均成功，不影响本次测试结论。

## 7. 软件边界

- `board/bsp/bsp_canfd.c`：仅保留端口配置、启停、收发、错误计数和 ISR 到软件队列的轻量封装。
- `app/can_test.c`：保存预设相关测试报文、非阻塞状态机、超时与结果校验。
- `board/board_config.h`：由用户选择三档 CAN 预设，并集中配置测试等待时间。

