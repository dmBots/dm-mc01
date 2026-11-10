#include "bsp_user_key.h"

#include "board_config.h"
#include "mx_gpio_default.h"

#include <stddef.h>

static volatile uint32_t user_key_interrupt_count;
static volatile uint32_t user_key_last_trigger;

static void bsp_user_key_exti_callback(hal_exti_handle_t *hexti,
                                       hal_exti_trigger_t trigger)
{
    (void)hexti;
    user_key_last_trigger = (uint32_t)trigger;
    user_key_interrupt_count++;
}

bool bsp_user_key_init(void)
{
    user_key_interrupt_count = 0U;
    user_key_last_trigger = (uint32_t)HAL_EXTI_TRIGGER_NONE;

    return HAL_EXTI_RegisterTriggerCallback(mx_gpio_default_exti15_gethandle(),
                                             bsp_user_key_exti_callback) == HAL_OK;
}

bool bsp_user_key_is_pressed(void)
{
    return HAL_GPIO_ReadPin(BOARD_USER_KEY_GPIO_PORT, BOARD_USER_KEY_GPIO_PIN) ==
           BOARD_USER_KEY_ACTIVE_STATE;
}

void bsp_user_key_get_status(bsp_user_key_status_t *status)
{
    if (status == NULL)
    {
        return;
    }

    status->interrupt_count = user_key_interrupt_count;
    status->last_trigger = user_key_last_trigger;
    status->pressed = bsp_user_key_is_pressed();
}
