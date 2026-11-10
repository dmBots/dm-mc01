# MC01-RC TIM8 PWM/输入捕获外部回环测试报告

## 1. 测试结论

测试日期：2026-09-08

TIM8 两组 PWM/输入捕获外部回环测试通过：

- `PC6 / TIM8_CH1 (PWM)` → `PC7 / TIM8_CH2 (输入捕获)`：通过。
- `PB5 / TIM8_CH3 (PWM)` → `PC9 / TIM8_CH4 (输入捕获)`：通过。

首次 500 ms 测试窗口内，CH2 和 CH4 均通过 TIM8 Capture/Compare 中断捕获到 500 个上升沿；独立复位复测分别捕获到 499 个上升沿，均符合 1 kHz PWM 的预期值和允许范围。测试结束后 TIM8 计数器、捕获中断、四个通道和高级定时器主输出均已关闭。

## 2. 测试环境与配置

- 目标器件：STM32C5A3RGT6
- 定时器：TIM8
- TIM8 时钟：144 MHz
- 预分频：143，计数频率 1 MHz
- 自动重装值：999
- PWM 频率：1 kHz
- CH1/CH3 比较值：500，约 50% 占空比
- 计数模式：向上计数
- CH2/CH4 捕获极性：上升沿
- 捕获分频：DIV1
- 捕获滤波：FDIV1
- 测试时间：500 ms
- 允许计数范围：450 至 550
- 调试器：DAPLink，UID `D641959C00B43ECC`
- 下载工具：项目 `.venv` 中的 pyOCD
- 构建预设：`debug_GCC_STM32C5A3RGT6`

CubeMX 复查确认：PC6、PC7、PC9 使用 AF3，PB5 使用 AF13，并已开启 `TIM8 Capture Compare interrupt`。

## 3. 软件结构

- `board/bsp/bsp_tim8_pwm_capture.c`：轻量封装 TIM8 获取、双路 PWM/捕获启停、捕获统计读取和输入捕获回调。
- `app/tim8_loopback_test.c`：保存测试状态，使用 `HAL_GetTick()` 执行一次性、非阻塞的 500 ms 回环判定。
- `board/board_config.h`：保存 PWM 频率、测试时长和计数容差。
- `main.c`：上电初始化测试，并在主循环中轮询测试状态机。

BSP 对用户层提供以下 API：

- `bsp_tim8_pwm_capture_init()`
- `bsp_tim8_pwm_capture_start()`
- `bsp_tim8_pwm_capture_stop()`
- `bsp_tim8_pwm_capture_get_status()`

捕获数据在 `HAL_TIM_InputCaptureCallback()` 中更新，主循环只读取结果并完成时间窗判定，没有轮询捕获标志。

## 4. 实板观测结果

### 4.1 测试运行中

测试状态机运行约 342 ms 时：

| 项目 | 观测值 | 说明 |
| --- | ---: | --- |
| CH2 捕获计数 | 341 | CH1→CH2 回环持续工作 |
| CH4 捕获计数 | 341 | CH3→CH4 回环持续工作 |
| TIM8 CR1 | `0x00000081` | `CEN=1`，计数器运行 |
| TIM8 DIER | `0x00000014` | CC2、CC4 捕获中断开启 |
| TIM8 CCER | `0x00001111` | CH1、CH2、CH3、CH4 均开启 |
| TIM8 BDTR | `0x00008000` | 高级定时器主输出 MOE 开启 |

### 4.2 测试结束

| 状态字段 | 实测值 |
| --- | ---: |
| `result` | 2（通过） |
| `failure_mask` | 0 |
| `elapsed_ms` | 500 |
| `expected_edge_count` | 500 |
| `ch2_edge_count` | 499 |
| `ch4_edge_count` | 499 |
| `ch2_last_capture_ticks` | 0 |
| `ch4_last_capture_ticks` | 0 |

停止状态寄存器：

| 项目 | 观测值 | 说明 |
| --- | ---: | --- |
| TIM8 CR1 | `0x00000080` | `CEN=0` |
| TIM8 DIER | `0x00000000` | 捕获中断关闭 |
| TIM8 CCER | `0x00000000` | 四个通道关闭 |
| TIM8 BDTR | `0x00000000` | MOE 关闭 |

输入和输出使用同一个 TIM8 时基，因此 PWM 上升沿位于计数周期起点，最后捕获值为 0 符合预期。

首次测试为 CH2=500、CH4=500；上表记录的是独立复位后的第二次结果 CH2=499、CH4=499。两次 `failure_mask` 均为 0。

## 5. 失败掩码定义

- bit 0：CH1→CH2 回环捕获计数不在允许范围。
- bit 1：CH3→CH4 回环捕获计数不在允许范围。
- bit 2：BSP 初始化、启停或状态读取失败。

## 6. 结论边界

本次外部回环覆盖了 PC6、PC7、PB5、PC9 的 GPIO 复用功能、TIM8 PWM 输出、TIM8 输入捕获、NVIC 中断入口、HAL 捕获回调以及软件启停流程。测试未使用示波器测量引脚电压、上升/下降时间和占空比模拟误差；若需要评估这些电气指标，应再使用示波器或逻辑分析仪测量。
