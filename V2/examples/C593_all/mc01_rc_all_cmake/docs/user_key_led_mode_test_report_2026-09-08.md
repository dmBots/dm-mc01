# USER_KEY 三档 LED 闪烁测试报告

## 1. 测试结论

- 测试日期：2026-09-08
- 测试对象：MC01-RC 控制板，STM32C5A3RGT6
- 调试下载：DAPLink + pyOCD
- 固件构建：`debug_GCC_STM32C5A3RGT6`
- 总体结果：通过

用户按键 PA15 采用 EXTI 双边沿中断，按键中断仅记录事件；30 ms 消抖、按下确认、三档切换和 LED 节拍均在应用轮询中完成。实板完成了标准、快、慢三档的完整循环，未发现初始化错误或误切档。

## 2. 原理图与配置核对

原理图 `CtrBoard-V2-260810A.pdf` 第 2 页显示：SW1 一端接地，USER_KEY(PA15) 通过 R61 上拉到 3.3 V。因此按键空闲为高、按下为低，与实板 GPIOA IDR 读数及软件配置一致。

CubeMX 生成配置核对结果：

| 项目 | 配置 |
| --- | --- |
| 用户按键 | PA15 / USER_KEY |
| GPIO 模式 | 输入，无内部上下拉；板上 R61 外部上拉 |
| EXTI | Line 15，中断模式，上升沿 + 下降沿 |
| 用户 LED | PC0，低电平点亮 |
| 消抖时间 | 30 ms |

## 3. 三档定义

以下时间均为 LED 电平翻转间隔，完整亮灭周期为其两倍：

| 模式编号 | 模式 | 翻转间隔 | 完整周期 |
| ---: | --- | ---: | ---: |
| 0 | 慢闪 | 1000 ms | 2000 ms |
| 1 | 标准闪烁（默认） | 500 ms | 1000 ms |
| 2 | 快闪 | 100 ms | 200 ms |

按键每次确认按下后按 `标准 → 快 → 慢 → 标准` 循环切换。周期、有效电平和消抖参数统一位于 `board/board_config.h`。

## 4. 软件结构

- `board/bsp/bsp_led.c/.h`：仅封装 LED 开关、翻转和状态读取。
- `board/bsp/bsp_user_key.c/.h`：注册 EXTI15 回调，提供按键电平与中断状态读取；中断中不做延时和业务处理。
- `app/led_mode_test.c/.h`：轮询完成按键消抖、仅按下切档、三档 LED 节拍和可调试状态记录。
- `main.c`：初始化一次，并在主循环调用 `led_mode_test_process()`；没有阻塞式延时。

## 5. 构建与下载

构建命令：

```powershell
cmake --build --preset debug_GCC_STM32C5A3RGT6 --parallel
```

结果：成功链接 `mc01_rc_all.elf`，新增源文件均参与编译，无编译错误。

下载命令：

```powershell
.\.venv\Scripts\pyocd.exe load --config .\pyocd.yaml --uid D641959C00B43ECC --format elf .\build\debug_GCC_STM32C5A3RGT6\mc01_rc_all.elf
```

结果：成功擦除并写入 155648 bytes，目标芯片识别为 STM32C5A3RGT6。

## 6. 实板按键切档结果

通过 pyOCD 读取 `led_mode_test_status`，得到同一次复位后的连续结果：

| 操作 | EXTI 边沿累计 | 有效按下累计 | 模式 | 间隔 | 消抖拒绝 |
| --- | ---: | ---: | --- | ---: | ---: |
| 第 1 次短按并释放 | 2 | 1 | 快闪 | 100 ms | 0 |
| 第 2 次短按并释放 | 4 | 2 | 慢闪 | 1000 ms | 0 |
| 第 3 次短按并释放 | 6 | 3 | 标准闪烁 | 500 ms | 0 |

每次短按均观察到下降沿和上升沿各一次，应用只在消抖后确认的按下沿切换模式，释放不切档。

## 7. LED 节拍核对

| 模式 | 观测时间 | 翻转计数增量 | 计算间隔 | 结果 |
| --- | ---: | ---: | ---: | --- |
| 快闪 | 11400 ms | 114 | 100 ms | 通过 |
| 慢闪 | 23000 ms | 23 | 1000 ms | 通过 |
| 标准闪烁 | 23500 ms | 47 | 500 ms | 通过 |

计时采用无阻塞轮询和无符号时间差，较长的主循环间隔通过累计跨过的节拍数修正基准时间，避免持续漂移。

## 8. 回归检查

最终固件运行状态读取结果：

| 模块 | 状态 |
| --- | --- |
| CAN（CANFD 8 Mbit/s 预设） | PASSED，passed_mask = 0x0F |
| RS485 2 Mbaud 外部 Echo | PASSED，passed_mask = 0x3F，错误/丢包为 0 |
| USART1/USART3 单口回环 | PASSED，passed_mask = 0x3F，错误/丢包为 0 |
| VBAT ADC1 DMA | PASSED，100 个样本 |
| BMI088 | PASSED，持续采样 |
| IMU 加热 | PASSED，无故障 |
| TIM8 PWM 捕获回环 | PASSED，failure_mask = 0 |
| ST7789 LCD + ADC2 五向键显示 | PASSED，failure_mask = 0 |

## 9. 最终判定

USER_KEY 的中断采集、应用层消抖、三档 LED 轮询切换、实板下载和回归测试均通过。当前板上运行于标准 500 ms 翻转间隔；再次短按将进入快闪档。
