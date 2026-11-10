#include "bsp_led.h"

void bsp_led_toggle(void)
{
    HAL_GPIO_TogglePin(BOARD_USER_LED_GPIO_PORT, BOARD_USER_LED_GPIO_PIN);
}

bool bsp_led_get_state(void)
{
    return HAL_GPIO_ReadPin(BOARD_USER_LED_GPIO_PORT, BOARD_USER_LED_GPIO_PIN) ==
           BOARD_USER_LED_ACTIVE_STATE;
}
