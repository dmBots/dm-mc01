# MC01-RC USB CDC 用户 API 测试报告

## 1. 测试结论

测试日期：2026-09-07

- USB CDC 用户侧非阻塞读写 API：通过编译与实板传输验证。
- CDC Echo：通过。USB OUT 接收的数据可从 CDC IN 端点逐字节原样返回。
- VOFA+ JustFloat 数据流：协议层通过。连续帧、帧尾、幅值与正弦波周期均正确。
- VOFA+ 图形界面显示：本次自动化环境未暴露原生 Windows 应用控制接口，无法自动操作和截图确认曲线窗口；需在 VOFA+ 中选择 COM75 和 JustFloat 后人工确认显示。

## 2. 测试环境

- 目标器件：STM32C5A3RGT6
- USB 类：USBX standalone CDC ACM
- CDC OUT 端点：`0x01`，Full-Speed 最大包长 64 字节
- CDC IN 端点：`0x82`，Full-Speed 最大包长 64 字节
- Windows CDC 端口：COM75
- USB VID/PID：`0483:5710`
- 调试器：DAPLink
- 下载工具：项目 `.venv` 中的 pyOCD
- 构建预设：`debug_GCC_STM32C5A3RGT6`

## 3. 软件结构

- `board/bsp/bsp_usb_cdc.c`：保存 CDC ACM 实例，仅提供连接状态、非阻塞读取、单包异步写入和发送任务轮询。
- `app/usb_test.c`：包含 Echo 和 VOFA JustFloat 两个应用测试，不进入 BSP。
- `board/board_config.h`：选择 USB 测试模式并配置 VOFA 采样周期、正弦频率与幅值。
- `main.c`：轮询 USBX、USB CDC 发送任务和 USB 测试任务。

USB CDC 的用户 API：

- `bsp_usb_cdc_is_connected()`
- `bsp_usb_cdc_tx_busy()`
- `bsp_usb_cdc_read()`
- `bsp_usb_cdc_write()`
- `bsp_usb_cdc_process()`

## 4. Echo 测试

配置：

```c
#define BOARD_USB_TEST_MODE BOARD_USB_TEST_ECHO
```

测试流程：

1. 零警告编译并使用 pyOCD 下载。
2. Windows 打开 COM75。
3. 主机发送二进制数据到 CDC OUT。
4. 读取 CDC IN 返回数据并逐字节比较。

结果：

| 测试数据 | 发送长度 | 接收长度 | 比较结果 |
| --- | ---: | ---: | --- |
| `00 01 02 ... 3F` | 64 | 64 | 完全一致 |
| `00 FF 55 AA 7F 80 11` | 7 | 7 | 完全一致 |

结论：满包、短包、`0x00`、`0xFF` 和高位二进制数据均可正确回显。

## 5. VOFA+ JustFloat 测试

配置：

```c
#define BOARD_USB_TEST_MODE               BOARD_USB_TEST_VOFA_JUSTFLOAT
#define BOARD_USB_VOFA_SAMPLE_INTERVAL_MS (10U)
#define BOARD_USB_VOFA_SINE_FREQUENCY_HZ  (1.0F)
#define BOARD_USB_VOFA_SINE_AMPLITUDE     (1.0F)
```

每帧格式：

1. 4 字节 ARM 小端 IEEE-754 `float` 正弦采样值。
2. JustFloat 帧尾 `00 00 80 7F`。

串口原始数据验证结果：

- 连续读取：1608 字节
- 对齐后有效帧：201 帧
- 正确 JustFloat 帧尾：201/201
- 最小值：-1.0
- 最大值：1.0
- 正向过零周期：100 个采样
- 采样周期：10 ms
- 计算频率：1 Hz
- 起始连续采样示例：`0.125333, 0.187381, 0.248690, 0.309017, 0.368125, ...`

结论：固件输出符合 VOFA+ JustFloat 单通道正弦波格式和配置参数。

## 6. VOFA+ 使用步骤

1. 在 VOFA+ 新建或选择串口数据源。
2. 选择 COM75；波特率字段可保持 115200，USB CDC 实际吞吐不由该数值决定。
3. 协议引擎选择 `JustFloat`。
4. 打开串口。
5. 添加一个波形通道，即可观察幅值约为 -1 至 1、频率约为 1 Hz 的正弦曲线。

## 7. 最终状态

- 最终留在板上的模式：`BOARD_USB_TEST_VOFA_JUSTFLOAT`
- 最终工程：零警告编译成功并已下载运行
- 将 USB CDC API 迁移至 BSP 后已完成回归：Echo 边界帧 7/7 字节一致；VOFA 连续 101/101 帧有效，幅值仍为 -1.0 至 1.0
- pyOCD 的 DFP Flash sequence 警告会自动回退到 FLM，不影响下载和本次测试结果
