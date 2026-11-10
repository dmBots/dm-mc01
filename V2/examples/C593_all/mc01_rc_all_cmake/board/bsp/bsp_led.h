/*
 * 控制板载 LED 指示灯；GPIO 是芯片的输入/输出引脚。
 * 本板 LED 通过低电平点亮，下面的函数已处理电平转换，调用时只需考虑亮或灭。
 */
#ifndef BSP_LED_H
#define BSP_LED_H

#include "board_config.h"
#include "stm32_hal.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/* bsp_led_set_state(true) 点亮，false 熄灭；toggle 每调用一次就切换一次亮灭。 */
static inline void bsp_led_set_state(bool state)
{
    hal_gpio_pin_state_t pin_state = state ? BOARD_USER_LED_ACTIVE_STATE :
                                             BOARD_USER_LED_INACTIVE_STATE;
    HAL_GPIO_WritePin(BOARD_USER_LED_GPIO_PORT, BOARD_USER_LED_GPIO_PIN, pin_state);
}

void bsp_led_toggle(void);
bool bsp_led_get_state(void);
#ifdef __cplusplus
}
#endif


#endif
