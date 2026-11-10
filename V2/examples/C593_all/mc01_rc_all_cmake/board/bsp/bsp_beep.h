/*
 * 控制板载蜂鸣器发声，使用芯片内的 TIM3 定时器 CH3 通道输出周期信号（PWM）。
 * set_frequency 设置音调频率，单位 Hz；例如 4000 表示每秒 4000 个周期。
 */
#ifndef BSP_BEEP_H
#define BSP_BEEP_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* init 只做发声准备，不会立即响；start 开始发声，stop 停止发声。 */
bool bsp_beep_init(void);
/* 频率设为 0 不表示静音，应调用 stop；频率无法设置时返回 false。 */
bool bsp_beep_set_frequency(uint32_t frequency_hz);
bool bsp_beep_start(void);
bool bsp_beep_stop(void);

#ifdef __cplusplus
}
#endif

#endif
