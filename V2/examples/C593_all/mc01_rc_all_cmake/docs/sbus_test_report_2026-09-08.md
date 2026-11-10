# UART7 SBUS 循环 DMA 接收与解析测试报告

## 1. 测试结论

UART7 的 SBUS 循环 DMA 接收、软件队列、帧同步和 16 通道解析测试通过。

板上连续接收 11,575 字节，解析出 462 个合法 SBUS 帧，没有坏帧、UART 错误、队列丢包或 DMA 重启。DMA 半满和全满中断均持续触发，证明数据通过中断路径进入软件队列。

当前接收机发送的每个有效帧均带 `frame_lost` 标志，`failsafe` 标志未置位。这是接收机上报的遥控链路状态，不是 MCU 的 UART/DMA 丢包。UART7 接收和协议解析功能已经通过，但遥控发射机与接收机的无线链路仍需检查。

## 2. 测试环境

- 目标芯片：STM32C5A3RGT6
- 接收外设：UART7 RX，PA8
- 调试器：DAPLink
- 下载与调试工具：pyOCD
- 固件：`build/debug_GCC_STM32C5A3RGT6/mc01_rc_all.elf`
- 测试日期：2026-09-08
- UART 波特率：100000 baud
- UART 帧格式：8 个有效数据位、偶校验、2 个停止位
- STM32 HAL 字长配置：`HAL_UART_WORD_LENGTH_9_BIT` 与 `HAL_UART_PARITY_EVEN`
- 信号反相：板上硬件反相器，软件不启用 RX 反相
- DMA：LPDMA2 Channel 0，外设到内存，循环链表模式
- DMA 缓冲区：50 字节
- 软件接收队列：256 字节

## 3. 配置问题与修正

### 3.1 UART 字长

首次生成配置为 `HAL_UART_WORD_LENGTH_8_BIT + HAL_UART_PARITY_EVEN`。在 STM32 UART 中，启用校验后校验位占用所配置字长的最高位，因此该组合只有 7 个有效数据位，不能完整接收 SBUS 的 8-bit 数据。

首次板上结果：

- 运行时间：约 1.5 秒；
- UART 错误：9,939；
- DMA 重启：9,939；
- 入队原始字节：0；
- 有效帧：0。

将字长改为 `HAL_UART_WORD_LENGTH_9_BIT` 后，UART 可以获取完整 8-bit SBUS 数据。

### 3.2 Receiver Timeout

UART7 生成配置启用了 250-bit Receiver Timeout。STM32 HAL 在启动普通 `HAL_UART_Receive_DMA()` 时，如果硬件 Receiver Timeout 已启用，会同时启用 RTO 中断；SBUS 帧间间隔随后被 HAL 作为接收错误处理，使 DMA 每帧重启。

关闭 Receiver Timeout 前的板上结果：

- 原始字节：9,825；
- 合法帧：393；
- UART 错误：393；
- DMA 重启：393。

`bsp_sbus_start()` 现在会在启动循环 DMA 前关闭 Receiver Timeout。SBUS 固定长度连续帧使用 DMA 半满和全满中断即可，不依赖接收超时。

建议在 CubeMX 中同时将 UART7 Word Length 设置为 9 Bits，并关闭 Receiver Timeout，避免重新生成代码时恢复不合适的配置。

## 4. BSP 用户接口

轻量接口位于 `board/bsp/bsp_sbus.h`：

- `bsp_sbus_start()`：使用 CubeMX 的 UART7 配置并启动循环 RX DMA；
- `bsp_sbus_process()`：在主循环中处理 UART 错误后的 DMA 恢复；
- `bsp_sbus_read()`：从软件队列读取原始 SBUS 字节；
- `bsp_sbus_rx_pending()`：查询队列中的待处理字节数；
- `bsp_sbus_get_status()`：读取 DMA 中断、UART 错误、队列丢包和重启计数；
- `bsp_sbus_stop()`：停止 UART7 接收。

BSP 不包含 SBUS 协议解析和测试状态机，便于用户直接复制；具体协议逻辑位于 `app/sbus_test.c`。

## 5. DMA 与队列路径

DMA 缓冲区长度为 50 字节，等于两个 SBUS 帧：

- 接收 25 字节时触发 DMA 半满中断，提交前半区；
- 接收 50 字节时触发 DMA 全满中断，提交后半区并回卷；
- 中断只将新增字节增量写入软件队列；
- 主循环从队列取出字节并执行帧同步及解析。

DMA 起点不要求与 SBUS 帧头对齐，解析器会在连续字节流中搜索 `0x0F` 帧头，并在帧校验失败时继续寻找下一个候选帧头。

## 6. 协议解析内容

应用解析以下数据：

- 25 字节固定帧；
- 字节 0：`0x0F` 帧头；
- 字节 1 至 22：16 路 11-bit 通道数据；
- 字节 23：CH17、CH18、frame-lost 和 failsafe 标志；
- 字节 24：帧尾；
- 支持 `0x00` 帧尾和低 4 位为 `0x04` 的常见帧尾变体。

应用启动时还会构造包含 16 个不同通道值的已知数据帧，对打包与解包结果逐通道比较，验证 176-bit 通道数据解析算法。

## 7. 最终板上结果

| 测试项 | 结果 |
| --- | ---: |
| 应用结果 | PASSED |
| 解析器已知向量自检 | 通过 |
| 原始接收字节 | 11,575 |
| 合法 SBUS 帧 | 462 |
| 非法帧 | 0 |
| 初始同步丢弃字节 | 3 |
| DMA 半满中断 | 232 |
| DMA 全满中断 | 231 |
| UART 错误 | 0 |
| 软件队列丢包 | 0 |
| DMA 错误重启 | 0 |
| 有信号 | 是 |

最近一帧原始数据：

```text
0F E0 03 1F F8 C0 07 3E 60 00 03 18 C0 00 06 00 00 00 00 00 00 00 00 04 00
```

最近一帧解析结果：

| 字段 | 值 |
| --- | --- |
| CH1 至 CH5 | 992, 992, 992, 992, 992 |
| CH6 至 CH10 | 192, 192, 192, 192, 192 |
| CH11 至 CH16 | 0, 0, 0, 0, 0, 0 |
| 数字 CH17 | 0 |
| 数字 CH18 | 0 |
| frame_lost | 1 |
| failsafe | 0 |

462 个合法帧中的 `frame_lost` 均为 1。建议检查遥控发射机是否开机、接收机是否正确对频、接收机输出口是否配置为 SBUS，以及发射机当前模型和接收协议是否匹配。在该标志恢复正常前，不应把通道值用于实际运动控制。

## 8. 回归结果

本次扩展了共享 HAL UART 回调分发，因此同步检查了已有功能：

- CAN 当前 FD8M 预设：通过掩码 `0x0F`，错误计数为 0；
- RS485 2 Mbaud 外部 Echo：通过掩码 `0x3F`，错误和丢包均为 0；
- USART1 / USART3 独立自回环：通过掩码 `0x3F`，错误和丢包均为 0。

## 9. 最终判定

UART7 SBUS 用户 BSP、循环 DMA 半满/全满入队、错误恢复、连续字节流同步、16 通道解析和标志位解析均已在真实硬件上验证通过。当前剩余问题属于接收机报告的遥控链路状态，不属于 MCU 接收实现。

## 10. CubeMX 重新生成后复测

用户使用 CubeMX 重新配置并生成 UART7 代码后，于 2026-09-08 再次执行完整编译、DAPLink 下载和板上测试。

重新生成后的 UART7 配置确认如下：

- 100000 baud；
- `HAL_UART_WORD_LENGTH_9_BIT`；
- 偶校验；
- 2 个停止位；
- 仅接收；
- Receiver Timeout 未启用；
- LPDMA2 Channel 0 循环链表接收。

复测结果：

| 测试项 | 复测结果 |
| --- | ---: |
| 应用结果 | PASSED |
| 解析器已知向量自检 | 通过 |
| 原始接收字节 | 10,975 |
| 合法 SBUS 帧 | 439 |
| 非法帧 | 0 |
| 同步丢弃字节 | 0 |
| DMA 半满中断 | 220 |
| DMA 全满中断 | 219 |
| UART 错误 | 0 |
| 软件队列丢包 | 0 |
| DMA 错误重启 | 0 |
| CAN FD8M 回归 | `0x0F`，通过 |
| RS485 回归 | `0x3F`，通过 |
| USART1 / USART3 回归 | `0x3F`，通过 |

CubeMX 重新生成没有覆盖 BSP、APP、CMake 或主循环接入，生成后的 UART7 配置可以直接稳定运行，不再依赖手工修改生成文件。

复测期间 439 个合法帧仍全部带 `frame_lost=1`，`failsafe=0`，因此遥控无线链路检查项保持不变。
