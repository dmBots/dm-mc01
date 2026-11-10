/*
 * 打开或关闭板上的两路 5 V 供电：外部设备供电、加热器供电。
 * 这里只控制电源开关，不会测量输出电压是否真的正常。
 */
#ifndef BSP_PWR_H
#define BSP_PWR_H

#include "board_config.h"
#include "stm32_hal.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif



/* bsp_pwr_outer_5v_cmd(true) 打开外设 5 V，false 关闭。 */
static inline void bsp_pwr_outer_5v_cmd(bool state)
{
    hal_gpio_pin_state_t pin_state=state?HAL_GPIO_PIN_SET:HAL_GPIO_PIN_RESET;
    HAL_GPIO_WritePin(BOARD_OUTER_POWER_5V_GPIO_PORT,BOARD_OUTER_POWER_5V_GPIO_PIN,pin_state);
}


/* true 打开加热器供电，false 关闭；仅打开电源不会自动调节温度。 */
static inline void bsp_pwr_heat_5v_cmd(bool state)
{
    hal_gpio_pin_state_t pin_state=state?HAL_GPIO_PIN_SET:HAL_GPIO_PIN_RESET;
    HAL_GPIO_WritePin(BOARD_HEAT_POWER_5V_GPIO_PORT,BOARD_HEAT_POWER_5V_GPIO_PIN,pin_state);
}


#ifdef __cplusplus
}
#endif


#endif
