# MC01-RC VBAT ADC DMA 采集实板测试报告

## 1. 测试结论

ADC1 Channel 0 的 VBAT 异步 DMA 采集测试通过。

- 连续完成 100 次单次 DMA 采样，DMA 完成计数为 100。
- ADC 错误计数为 0，采样超时计数为 0。
- ADC 原始码均值为 2647，最小值为 2643，最大值为 2655，峰峰值为 12 LSB。
- 按原理图分压参数换算，VBAT 平均值为 23.464 V，采样范围为 23.429 V 至 23.535 V。
- 测试结果、采样次数、电压范围和稳定性检查均通过。

## 2. 测试环境

- MCU：STM32C5A3RGT6
- ADC 输入：ADC1_IN0 / PA0 / `VBAT_ADC_PIN`
- 供电状态：项目当前约 24 V 外部输入
- 下载器：DAPLink，UID `D641959C00B43ECC`
- 下载及调试工具：项目 `.venv` 中的 pyOCD
- 构建预设：`debug_GCC_STM32C5A3RGT6`
- 测试日期：2026-09-08

pyOCD 下载时提示 DFP 缺少 Flash programming debug sequence，并自动回退到 FLM。固件擦除和编程成功，该提示不影响本次测试结论。

## 3. 硬件换算依据

根据 `CtrBoard-V2-260810A.pdf` 中 VBAT 采样网络：

- 上拉电阻 R86：100 kΩ
- 下拉电阻 R87：10 kΩ
- 滤波电容 C53：100 nF
- 分压倍率：`(100 kΩ + 10 kΩ) / 10 kΩ = 11`

固件采用以下标称换算：

```text
ADC_PIN_mV = RAW × 3300 / 4095
VBAT_mV    = RAW × 3300 × 11 / 4095
```

本次结果属于基于 3.3 V 标称 ADC 参考电压及标称电阻值的板上功能验证；若需要计量级精度，应增加参考电压实测、电阻误差和 ADC 增益/偏置校准。

## 4. CubeMX 配置确认

- ADC 分辨率：12 bit
- Regular sequencer：单通道、单次转换、软件触发
- 通道：ADC1_IN0，Rank 1，单端输入
- 采样时间：289 ADC cycles
- Overrun：覆盖旧数据
- DMA：LPDMA2 Channel 1
- DMA 方向：Peripheral to Memory
- DMA 源/目标宽度：Word / Word

## 5. 软件结构

### 5.1 轻量 BSP API

`board/bsp/bsp_adc.h` 仅保留用户可直接复用的薄封装：

```c
bool bsp_adc_vbat_init(void);
void bsp_adc_vbat_process(void);
bool bsp_adc_vbat_start_sample(void);
bool bsp_adc_vbat_read_sample(uint32_t *raw, uint32_t *sequence);
bool bsp_adc_vbat_get_status(bsp_adc_vbat_status_t *status);
```

BSP 负责启动单次异步 DMA、接收 ADC/DMA 回调、在轮询上下文中停止本次 DMA 并发布采样值。中断回调只设置状态标志，不在中断中执行计算或测试逻辑。

### 5.2 APP 测试

`app/vbat_test.c` 使用非阻塞状态机完成：

1. 每 10 ms 启动一次异步 DMA 采样。
2. 轮询 BSP 并消费已完成的样本。
3. 累计 100 个样本，计算原始码最小值、最大值、均值和峰峰值。
4. 根据板级分压参数换算 ADC 引脚电压和 VBAT 电压。
5. 检查 DMA/ADC 错误、超时、饱和、电压范围和采样稳定性。

板级参数均集中在 `board/board_config.h`，包括 ADC 参考电压、12 bit 满量程、分压电阻、采样周期、超时、样本数量和通过阈值。

## 6. 构建与下载

工程完整编译成功，链接完成，未观察到编译错误或警告。随后使用 DAPLink + pyOCD 下载 ELF，擦除 114688 bytes、编程 114688 bytes，下载成功。

下载后复位运行，通过 SWD 读取 `vbat_test_status` 状态区。由于 AIO 初始化包含 BMI088 启动延时，等待初始化及 100 次 ADC 采样完成后再读取最终状态。

## 7. 实板结果

| 状态字段 | 实测值 |
| --- | ---: |
| `result` | 2（PASSED） |
| `phase` | 2（COMPLETE） |
| `failure_code` | 0 |
| `sample_count` | 100 |
| `dma_complete_count` | 100 |
| `adc_error_count` | 0 |
| `timeout_count` | 0 |
| `latest_raw` | 2645 |
| `minimum_raw` | 2643 |
| `maximum_raw` | 2655 |
| `average_raw` | 2647 |
| `raw_spread` | 12 LSB |
| `adc_pin_mv` | 2132 mV（最新样本） |
| `latest_vbat_mv` | 23447 mV |
| `minimum_vbat_mv` | 23429 mV |
| `maximum_vbat_mv` | 23535 mV |
| `average_vbat_mv` | 23464 mV |

测试判据为 VBAT 平均值处于 18 V 至 30 V、原始码峰峰值不超过 100 LSB，且不存在 ADC 错误或采样超时。本次全部满足。

## 8. AIO 回归观测

同一版固件运行时同步读取已有测试状态：

- CAN 当前预设测试：`result=2`、`phase=2`、`passed_mask=0x0F`，通过。
- RS485 2 Mbaud 外部 Echo：`result=2`、`passed_mask=0x3F`、错误及丢包计数为 0，通过。
- USART1/USART3 独立自回环：`result=2`、`passed_mask=0x3F`、错误及丢包计数为 0，通过。
- UART7 SBUS：`result=2`、解析器自检通过、接收测试通过；遥控器上报的 frame-lost 标志仍按既有报告记录，不属于本次 ADC 改动。

## 9. 最终判定

本次 CubeMX 重新生成后的 ADC1_IN0、LPDMA2 Channel 1 配置可正常工作。VBAT BSP 保持轻量，异步 DMA 的启动、完成回调、轮询发布及 APP 层统计换算链路均已在实板验证通过，可以作为后续 AIO 应用的 VBAT 采集接口使用。
