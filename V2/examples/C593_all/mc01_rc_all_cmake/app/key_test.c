#include "key_test.h"

#include <stddef.h>

#include "board_config.h"
#include "bsp_adc.h"

typedef struct
{
    uint32_t next_sample_time_ms;
    uint32_t sample_start_time_ms;
    uint32_t latest_sequence;
    bool sample_active;
} key_test_context_t;

static volatile key_test_summary_t key_test_status;
static key_test_context_t key_test_context;

static void key_test_fail(key_test_failure_t failure)
{
    key_test_status.result = KEY_TEST_RESULT_FAILED;
    key_test_status.failure_code = failure;
}

void key_test_init(uint32_t current_time_ms)
{
    key_test_status.result = KEY_TEST_RESULT_RUNNING;
    key_test_status.failure_code = KEY_TEST_FAILURE_NONE;
    key_test_status.sample_count = 0U;
    key_test_status.latest_raw = 0U;
    key_test_status.minimum_raw = 0U;
    key_test_status.maximum_raw = 0U;
    key_test_status.latest_pin_mv = 0U;
    key_test_status.dma_complete_count = 0U;
    key_test_status.adc_error_count = 0U;
    key_test_status.timeout_count = 0U;
    key_test_context.next_sample_time_ms = current_time_ms;
    key_test_context.sample_start_time_ms = current_time_ms;
    key_test_context.latest_sequence = 0U;
    key_test_context.sample_active = false;

    if (!bsp_adc_key_init())
    {
        key_test_fail(KEY_TEST_FAILURE_INITIALIZATION);
    }
}

void key_test_process(uint32_t current_time_ms)
{
    bsp_adc_key_status_t adc_status;
    uint32_t raw;
    uint32_t sequence;

    if ((key_test_status.result == KEY_TEST_RESULT_IDLE) ||
        (key_test_status.result == KEY_TEST_RESULT_FAILED))
    {
        return;
    }

    bsp_adc_key_process();
    if (!bsp_adc_key_get_status(&adc_status))
    {
        key_test_fail(KEY_TEST_FAILURE_ADC);
        return;
    }
    key_test_status.dma_complete_count = adc_status.dma_complete_count;
    key_test_status.adc_error_count = adc_status.error_count;
    if (adc_status.error_count != 0U)
    {
        key_test_fail(KEY_TEST_FAILURE_ADC);
        return;
    }

    if (bsp_adc_key_read_sample(&raw, &sequence))
    {
        key_test_context.sample_active = false;
        key_test_context.latest_sequence = sequence;
        key_test_status.latest_raw = raw;
        key_test_status.latest_pin_mv =
            (uint32_t)((((uint64_t)raw * BOARD_ADC_REFERENCE_MV) +
                        (BOARD_ADC_12BIT_FULL_SCALE / 2U)) /
                       BOARD_ADC_12BIT_FULL_SCALE);
        if ((key_test_status.sample_count == 0U) ||
            (raw < key_test_status.minimum_raw))
        {
            key_test_status.minimum_raw = raw;
        }
        if ((key_test_status.sample_count == 0U) ||
            (raw > key_test_status.maximum_raw))
        {
            key_test_status.maximum_raw = raw;
        }
        key_test_status.sample_count++;
        /* 读够次数只说明能够连续测量；每个按键位置是否正常，还要实际操作并观察屏幕数字。 */
        if ((key_test_status.result == KEY_TEST_RESULT_RUNNING) &&
            (key_test_status.sample_count >= BOARD_KEY_ADC_TEST_SAMPLE_COUNT))
        {
            key_test_status.result = KEY_TEST_RESULT_PASSED;
        }
        key_test_context.next_sample_time_ms =
            current_time_ms + BOARD_KEY_ADC_SAMPLE_INTERVAL_MS;
    }

    if (key_test_context.sample_active &&
        ((current_time_ms - key_test_context.sample_start_time_ms) >=
         BOARD_KEY_ADC_SAMPLE_TIMEOUT_MS))
    {
        key_test_status.timeout_count++;
        key_test_fail(KEY_TEST_FAILURE_TIMEOUT);
        return;
    }

    if (!key_test_context.sample_active &&
        ((int32_t)(current_time_ms - key_test_context.next_sample_time_ms) >= 0))
    {
        if (!bsp_adc_key_start_sample())
        {
            key_test_fail(KEY_TEST_FAILURE_START);
        }
        else
        {
            key_test_context.sample_active = true;
            key_test_context.sample_start_time_ms = current_time_ms;
        }
    }
}

bool key_test_get_latest(uint32_t *raw, uint32_t *sequence)
{
    if ((raw == NULL) || (sequence == NULL) ||
        (key_test_context.latest_sequence == 0U))
    {
        return false;
    }

    *raw = key_test_status.latest_raw;
    *sequence = key_test_context.latest_sequence;
    return true;
}

bool key_test_get_summary(key_test_summary_t *summary)
{
    if (summary == NULL)
    {
        return false;
    }

    *summary = key_test_status;
    return true;
}
