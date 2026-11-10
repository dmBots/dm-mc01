#include "bsp_beep.h"

#include "board_config.h"
#include "mx_tim3.h"

#define BSP_BEEP_CHANNEL        HAL_TIM_CHANNEL_3
#define BSP_BEEP_COMPARE_UNIT   HAL_TIM_OC_COMPARE_UNIT_3
#define BSP_BEEP_MAX_COUNTS     (65536UL)

static hal_tim_handle_t *beep_timer;
static bool beep_running;

bool bsp_beep_init(void)
{
    beep_timer = mx_tim3_gethandle();
    beep_running = false;

    return (beep_timer != NULL) &&
           bsp_beep_set_frequency(BOARD_BEEP_DEFAULT_FREQUENCY_HZ);
}

bool bsp_beep_set_frequency(uint32_t frequency_hz)
{
    uint32_t counter_frequency_hz;
    uint32_t prescaler;
    uint32_t period_counts;
    uint32_t pulse_counts;

    if ((beep_timer == NULL) || (frequency_hz == 0U))
    {
        return false;
    }

    prescaler = HAL_TIM_GetPrescaler(beep_timer);
    counter_frequency_hz = BOARD_BEEP_TIMER_CLOCK_HZ / (prescaler + 1U);
    if (frequency_hz > (counter_frequency_hz / 2U))
    {
        return false;
    }

    period_counts = (uint32_t)(((uint64_t)counter_frequency_hz + (frequency_hz / 2U)) /
                               frequency_hz);
    if ((period_counts < 2U) || (period_counts > BSP_BEEP_MAX_COUNTS))
    {
        return false;
    }

    pulse_counts = (period_counts * BOARD_BEEP_DUTY_PERCENT) / 100U;
    if (pulse_counts == 0U)
    {
        pulse_counts = 1U;
    }
    else if (pulse_counts >= period_counts)
    {
        pulse_counts = period_counts - 1U;
    }

    return (HAL_TIM_SetPeriod(beep_timer, period_counts - 1U) == HAL_OK) &&
           (HAL_TIM_OC_SetCompareUnitPulse(beep_timer, BSP_BEEP_COMPARE_UNIT,
                                           pulse_counts) == HAL_OK) &&
           (HAL_TIM_GenerateEvent(beep_timer, HAL_TIM_SW_EVENT_UPD) == HAL_OK);
}

bool bsp_beep_start(void)
{
    if (beep_timer == NULL)
    {
        return false;
    }
    if (beep_running)
    {
        return true;
    }
    if (HAL_TIM_Start(beep_timer) != HAL_OK)
    {
        return false;
    }
    if (HAL_TIM_OC_StartChannel(beep_timer, BSP_BEEP_CHANNEL) != HAL_OK)
    {
        (void)HAL_TIM_Stop(beep_timer);
        return false;
    }

    beep_running = true;
    return true;
}

bool bsp_beep_stop(void)
{
    bool channel_stopped;
    bool timer_stopped;

    if (!beep_running)
    {
        return true;
    }

    channel_stopped = HAL_TIM_OC_StopChannel(beep_timer, BSP_BEEP_CHANNEL) == HAL_OK;
    timer_stopped = HAL_TIM_Stop(beep_timer) == HAL_OK;
    beep_running = false;
    return channel_stopped && timer_stopped;
}
