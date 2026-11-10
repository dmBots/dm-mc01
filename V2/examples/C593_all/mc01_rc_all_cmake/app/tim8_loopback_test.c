#include "tim8_loopback_test.h"

#include <stddef.h>

#include "board_config.h"
#include "bsp_tim8_pwm_capture.h"

static volatile tim8_loopback_test_summary_t tim8_loopback_test_status;
static uint32_t test_start_time_ms;

static bool tim8_loopback_count_is_valid(uint32_t edge_count)
{
    uint32_t tolerance;
    uint32_t expected = tim8_loopback_test_status.expected_edge_count;

    tolerance = (expected * BOARD_TIM8_LOOPBACK_TOLERANCE_PERCENT) / 100U;
    return (edge_count >= (expected - tolerance)) &&
           (edge_count <= (expected + tolerance));
}

void tim8_loopback_test_init(uint32_t current_time_ms)
{
    tim8_loopback_test_status.result = TIM8_LOOPBACK_RESULT_RUNNING;
    tim8_loopback_test_status.failure_mask = 0U;
    tim8_loopback_test_status.elapsed_ms = 0U;
    tim8_loopback_test_status.expected_edge_count =
        (BOARD_TIM8_PWM_FREQUENCY_HZ * BOARD_TIM8_LOOPBACK_TEST_TIME_MS) / 1000U;
    tim8_loopback_test_status.ch2_edge_count = 0U;
    tim8_loopback_test_status.ch4_edge_count = 0U;
    tim8_loopback_test_status.ch2_last_capture_ticks = 0U;
    tim8_loopback_test_status.ch4_last_capture_ticks = 0U;
    test_start_time_ms = current_time_ms;

    if (!bsp_tim8_pwm_capture_init() || !bsp_tim8_pwm_capture_start())
    {
        tim8_loopback_test_status.failure_mask = TIM8_LOOPBACK_FAILURE_BSP;
        tim8_loopback_test_status.result = TIM8_LOOPBACK_RESULT_FAILED;
    }
}

void tim8_loopback_test_process(uint32_t current_time_ms)
{
    bsp_tim8_capture_status_t ch2_status = {0U};
    bsp_tim8_capture_status_t ch4_status = {0U};

    if (tim8_loopback_test_status.result != TIM8_LOOPBACK_RESULT_RUNNING)
    {
        return;
    }

    tim8_loopback_test_status.elapsed_ms = current_time_ms - test_start_time_ms;
    if (tim8_loopback_test_status.elapsed_ms < BOARD_TIM8_LOOPBACK_TEST_TIME_MS)
    {
        return;
    }

    if (!bsp_tim8_pwm_capture_stop())
    {
        tim8_loopback_test_status.failure_mask |= TIM8_LOOPBACK_FAILURE_BSP;
    }
    if (!bsp_tim8_pwm_capture_get_status(BSP_TIM8_CAPTURE_CH2, &ch2_status) ||
        !bsp_tim8_pwm_capture_get_status(BSP_TIM8_CAPTURE_CH4, &ch4_status))
    {
        tim8_loopback_test_status.failure_mask |= TIM8_LOOPBACK_FAILURE_BSP;
    }

    tim8_loopback_test_status.ch2_edge_count = ch2_status.edge_count;
    tim8_loopback_test_status.ch4_edge_count = ch4_status.edge_count;
    tim8_loopback_test_status.ch2_last_capture_ticks = ch2_status.last_capture_ticks;
    tim8_loopback_test_status.ch4_last_capture_ticks = ch4_status.last_capture_ticks;
    /* 分别检查两根跳线收到的脉冲数量；如果两路都不合格，failure_mask 会同时记下两路问题。 */
    if (!tim8_loopback_count_is_valid(ch2_status.edge_count))
    {
        tim8_loopback_test_status.failure_mask |= TIM8_LOOPBACK_FAILURE_CH1_TO_CH2;
    }
    if (!tim8_loopback_count_is_valid(ch4_status.edge_count))
    {
        tim8_loopback_test_status.failure_mask |= TIM8_LOOPBACK_FAILURE_CH3_TO_CH4;
    }

    tim8_loopback_test_status.result =
        (tim8_loopback_test_status.failure_mask == 0U) ?
        TIM8_LOOPBACK_RESULT_PASSED : TIM8_LOOPBACK_RESULT_FAILED;
}

bool tim8_loopback_test_get_summary(tim8_loopback_test_summary_t *summary)
{
    if (summary == NULL)
    {
        return false;
    }

    *summary = tim8_loopback_test_status;
    return true;
}
