/*
 * 用定时器 TIM8 产生周期脉冲，再从另外两个引脚接收，检查脉冲能否正确回来。
 * 按芯片引脚名接两根测试跳线：PC6（CH1）接 PC7（CH2），PB5（CH3）接 PC9（CH4）。
 * CH 表示定时器通道，不是板上 P 接口编号；PC6/PB5 是输出，PC7/PC9 是输入。
 */
#ifndef BSP_TIM8_PWM_CAPTURE_H
#define BSP_TIM8_PWM_CAPTURE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    BSP_TIM8_CAPTURE_CH2 = 0U,
    BSP_TIM8_CAPTURE_CH4
} bsp_tim8_capture_channel_t;

typedef struct
{
    uint32_t edge_count;
    uint32_t last_capture_ticks; /* 最近一次脉冲到来时的定时器读数，不是直接换算好的微秒数。 */
} bsp_tim8_capture_status_t;

bool bsp_tim8_pwm_capture_init(void);
/* init 做准备，start 开始发脉冲并计数，stop 停止；停止后仍可读到本轮计数。
 * 停止后再次 start 会把旧计数清零，开始新一轮统计。
 */
bool bsp_tim8_pwm_capture_start(void);
bool bsp_tim8_pwm_capture_stop(void);
bool bsp_tim8_pwm_capture_get_status(bsp_tim8_capture_channel_t channel,
                                     bsp_tim8_capture_status_t *status);

#ifdef __cplusplus
}
#endif

#endif
