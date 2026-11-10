#include "bsp_heater.h"

#include "mx_tim2.h"

#define BSP_HEATER_CHANNEL        HAL_TIM_CHANNEL_3
#define BSP_HEATER_COMPARE_UNIT   HAL_TIM_OC_COMPARE_UNIT_3
#define BSP_HEATER_DUTY_FULL_SCALE (1000U)

static hal_tim_handle_t *heater_timer;
static bool heater_running;

bool bsp_heater_init(void)
{
    heater_timer = mx_tim2_gethandle();
    heater_running = false;
    if ((heater_timer == NULL) ||
        (HAL_TIM_OC_SetCompareUnitPulse(heater_timer, BSP_HEATER_COMPARE_UNIT, 0U) != HAL_OK) ||
        (HAL_TIM_Start(heater_timer) != HAL_OK))
    {
        return false;
    }
    if (HAL_TIM_OC_StartChannel(heater_timer, BSP_HEATER_CHANNEL) != HAL_OK)
    {
        (void)HAL_TIM_Stop(heater_timer);
        return false;
    }

    heater_running = true;
    return true;
}

bool bsp_heater_set_duty_per_mille(uint16_t duty_per_mille)
{
    uint32_t period_counts;
    uint32_t pulse_counts;

    if ((heater_timer == NULL) || !heater_running ||
        (duty_per_mille > BSP_HEATER_DUTY_FULL_SCALE))
    {
        return false;
    }

    period_counts = HAL_TIM_GetPeriod(heater_timer) + 1U;
    pulse_counts = (period_counts * duty_per_mille) / BSP_HEATER_DUTY_FULL_SCALE;
    return HAL_TIM_OC_SetCompareUnitPulse(heater_timer, BSP_HEATER_COMPARE_UNIT,
                                          pulse_counts) == HAL_OK;
}

bool bsp_heater_shutdown(void)
{
    bool channel_stopped;
    bool timer_stopped;

    if ((heater_timer == NULL) || !heater_running)
    {
        return true;
    }

    (void)HAL_TIM_OC_SetCompareUnitPulse(heater_timer, BSP_HEATER_COMPARE_UNIT, 0U);
    channel_stopped = HAL_TIM_OC_StopChannel(heater_timer, BSP_HEATER_CHANNEL) == HAL_OK;
    timer_stopped = HAL_TIM_Stop(heater_timer) == HAL_OK;
    heater_running = false;
    return channel_stopped && timer_stopped;
}
