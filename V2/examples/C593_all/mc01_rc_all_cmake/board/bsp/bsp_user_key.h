/*
 * 读取板载用户按键；不要与 app/key_test.h 的模拟按键电压采样混淆。
 * EXTI 是引脚电平变化时自动通知程序的中断；一次按键动作可能触发多次通知。
 * 这里记录原始状态，判断按键是否稳定按下的消抖处理在 app/led_mode_test.c。
 */
#ifndef BSP_USER_KEY_H
#define BSP_USER_KEY_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint32_t interrupt_count;
    uint32_t last_trigger;
    bool pressed;
} bsp_user_key_status_t;

bool bsp_user_key_init(void);
/* 按下返回 true，松开返回 false，不需要调用者自己判断高低电平。 */
bool bsp_user_key_is_pressed(void);
void bsp_user_key_get_status(bsp_user_key_status_t *status);

#endif
