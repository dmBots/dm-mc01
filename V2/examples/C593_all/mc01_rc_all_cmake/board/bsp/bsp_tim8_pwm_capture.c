#include "bsp_tim8_pwm_capture.h"

#include "mx_tim8.h"

#define BSP_TIM8_PWM_CHANNEL_1       HAL_TIM_CHANNEL_1
#define BSP_TIM8_CAPTURE_CHANNEL_2   HAL_TIM_CHANNEL_2
#define BSP_TIM8_PWM_CHANNEL_3       HAL_TIM_CHANNEL_3
#define BSP_TIM8_CAPTURE_CHANNEL_4   HAL_TIM_CHANNEL_4

typedef struct
{
    volatile uint32_t edge_count;
    volatile uint32_t last_capture_ticks;
} bsp_tim8_capture_context_t;

static hal_tim_handle_t *tim8_handle;
static bsp_tim8_capture_context_t capture_context[2];
static bool tim8_running;

static void bsp_tim8_pwm_capture_reset_status(void)
{
    capture_context[BSP_TIM8_CAPTURE_CH2].edge_count = 0U;
    capture_context[BSP_TIM8_CAPTURE_CH2].last_capture_ticks = 0U;
    capture_context[BSP_TIM8_CAPTURE_CH4].edge_count = 0U;
    capture_context[BSP_TIM8_CAPTURE_CH4].last_capture_ticks = 0U;
}

bool bsp_tim8_pwm_capture_init(void)
{
    tim8_handle = mx_tim8_gethandle();
    tim8_running = false;
    bsp_tim8_pwm_capture_reset_status();
    return tim8_handle != NULL;
}

bool bsp_tim8_pwm_capture_start(void)
{
    bool capture_ch2_started = false;
    bool capture_ch4_started = false;
    bool pwm_ch1_started = false;
    bool pwm_ch3_started = false;

    if (tim8_handle == NULL)
    {
        return false;
    }
    if (tim8_running)
    {
        return true;
    }

    bsp_tim8_pwm_capture_reset_status();
    if ((HAL_TIM_SetCounter(tim8_handle, 0U) != HAL_OK) ||
        (HAL_TIM_GenerateEvent(tim8_handle, HAL_TIM_SW_EVENT_UPD) != HAL_OK))
    {
        return false;
    }

    capture_ch2_started =
        HAL_TIM_IC_StartChannel_IT(tim8_handle, BSP_TIM8_CAPTURE_CHANNEL_2) == HAL_OK;
    capture_ch4_started = capture_ch2_started &&
        (HAL_TIM_IC_StartChannel_IT(tim8_handle, BSP_TIM8_CAPTURE_CHANNEL_4) == HAL_OK);
    pwm_ch1_started = capture_ch4_started &&
        (HAL_TIM_OC_StartChannel(tim8_handle, BSP_TIM8_PWM_CHANNEL_1) == HAL_OK);
    pwm_ch3_started = pwm_ch1_started &&
        (HAL_TIM_OC_StartChannel(tim8_handle, BSP_TIM8_PWM_CHANNEL_3) == HAL_OK);

    if (!pwm_ch3_started || (HAL_TIM_Start(tim8_handle) != HAL_OK))
    {
        if (pwm_ch3_started)
        {
            (void)HAL_TIM_OC_StopChannel(tim8_handle, BSP_TIM8_PWM_CHANNEL_3);
        }
        if (pwm_ch1_started)
        {
            (void)HAL_TIM_OC_StopChannel(tim8_handle, BSP_TIM8_PWM_CHANNEL_1);
        }
        if (capture_ch4_started)
        {
            (void)HAL_TIM_IC_StopChannel_IT(tim8_handle, BSP_TIM8_CAPTURE_CHANNEL_4);
        }
        if (capture_ch2_started)
        {
            (void)HAL_TIM_IC_StopChannel_IT(tim8_handle, BSP_TIM8_CAPTURE_CHANNEL_2);
        }
        return false;
    }

    tim8_running = true;
    return true;
}

bool bsp_tim8_pwm_capture_stop(void)
{
    bool result = true;

    if (!tim8_running)
    {
        return true;
    }

    if (HAL_TIM_Stop(tim8_handle) != HAL_OK)
    {
        result = false;
    }
    if (HAL_TIM_IC_StopChannel_IT(tim8_handle, BSP_TIM8_CAPTURE_CHANNEL_2) != HAL_OK)
    {
        result = false;
    }
    if (HAL_TIM_IC_StopChannel_IT(tim8_handle, BSP_TIM8_CAPTURE_CHANNEL_4) != HAL_OK)
    {
        result = false;
    }
    if (HAL_TIM_OC_StopChannel(tim8_handle, BSP_TIM8_PWM_CHANNEL_1) != HAL_OK)
    {
        result = false;
    }
    if (HAL_TIM_OC_StopChannel(tim8_handle, BSP_TIM8_PWM_CHANNEL_3) != HAL_OK)
    {
        result = false;
    }

    tim8_running = false;
    return result;
}

bool bsp_tim8_pwm_capture_get_status(bsp_tim8_capture_channel_t channel,
                                     bsp_tim8_capture_status_t *status)
{
    uint32_t first_count;
    uint32_t second_count;

    if ((status == NULL) || (channel > BSP_TIM8_CAPTURE_CH4))
    {
        return false;
    }

    /* 读数时可能正好来了新脉冲；前后两次计数一致，才认为取到的是同一时刻的一组结果。 */
    do
    {
        first_count = capture_context[channel].edge_count;
        status->last_capture_ticks = capture_context[channel].last_capture_ticks;
        second_count = capture_context[channel].edge_count;
    } while (first_count != second_count);

    status->edge_count = second_count;
    return true;
}

void HAL_TIM_InputCaptureCallback(hal_tim_handle_t *htim, hal_tim_channel_t channel)
{
    bsp_tim8_capture_context_t *context;

    if (htim != tim8_handle)
    {
        return;
    }

    if (channel == BSP_TIM8_CAPTURE_CHANNEL_2)
    {
        context = &capture_context[BSP_TIM8_CAPTURE_CH2];
    }
    else if (channel == BSP_TIM8_CAPTURE_CHANNEL_4)
    {
        context = &capture_context[BSP_TIM8_CAPTURE_CH4];
    }
    else
    {
        return;
    }

    context->last_capture_ticks = HAL_TIM_IC_ReadChannelCapturedValue(htim, channel);
    context->edge_count++;
}
