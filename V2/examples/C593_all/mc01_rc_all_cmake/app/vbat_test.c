#include "vbat_test.h"

#include <stdbool.h>
#include <stddef.h>

#include "board_config.h"
#include "bsp_adc.h"

typedef enum
{
    VBAT_TEST_PHASE_IDLE = 0U,
    VBAT_TEST_PHASE_COLLECT,
    VBAT_TEST_PHASE_COMPLETE,
    VBAT_TEST_PHASE_FAILED
} vbat_test_phase_t;

typedef struct
{
    uint32_t next_sample_time_ms;
    uint32_t sample_start_time_ms;
    uint32_t raw_sum;
    bool sample_active;
} vbat_test_context_t;

static volatile vbat_test_summary_t vbat_test_status;
static vbat_test_context_t vbat_test_context;

static uint32_t vbat_test_raw_to_pin_mv(uint32_t raw)
{
    return (uint32_t)((((uint64_t)raw * BOARD_ADC_REFERENCE_MV) +
                       (BOARD_ADC_12BIT_FULL_SCALE / 2U)) /
                      BOARD_ADC_12BIT_FULL_SCALE);
}

static uint32_t vbat_test_raw_to_vbat_mv(uint32_t raw)
{
    uint64_t numerator = (uint64_t)raw * BOARD_ADC_REFERENCE_MV *
                         (BOARD_VBAT_DIVIDER_HIGH_OHM + BOARD_VBAT_DIVIDER_LOW_OHM);
    uint32_t denominator = BOARD_ADC_12BIT_FULL_SCALE * BOARD_VBAT_DIVIDER_LOW_OHM;

    return (uint32_t)((numerator + (denominator / 2U)) / denominator);
}

static void vbat_test_fail(vbat_test_failure_t failure)
{
    vbat_test_status.result = VBAT_TEST_RESULT_FAILED;
    vbat_test_status.phase = VBAT_TEST_PHASE_FAILED;
    vbat_test_status.failure_code = failure;
}

static void vbat_test_update_sample(uint32_t raw)
{
    uint32_t vbat_mv = vbat_test_raw_to_vbat_mv(raw);

    vbat_test_status.latest_raw = raw;
    vbat_test_status.adc_pin_mv = vbat_test_raw_to_pin_mv(raw);
    vbat_test_status.latest_vbat_mv = vbat_mv;
    if ((vbat_test_status.sample_count == 0U) || (raw < vbat_test_status.minimum_raw))
    {
        vbat_test_status.minimum_raw = raw;
        vbat_test_status.minimum_vbat_mv = vbat_mv;
    }
    if ((vbat_test_status.sample_count == 0U) || (raw > vbat_test_status.maximum_raw))
    {
        vbat_test_status.maximum_raw = raw;
        vbat_test_status.maximum_vbat_mv = vbat_mv;
    }
    vbat_test_context.raw_sum += raw;
    vbat_test_status.sample_count++;
}

/* 是否电压合格看换算后的毫伏值；是否稳定看最大读数与最小读数的差，两项不要混用单位。 */
static void vbat_test_finish(void)
{
    vbat_test_status.average_raw =
        (vbat_test_context.raw_sum + (vbat_test_status.sample_count / 2U)) /
        vbat_test_status.sample_count;
    vbat_test_status.raw_spread =
        vbat_test_status.maximum_raw - vbat_test_status.minimum_raw;
    vbat_test_status.average_vbat_mv =
        vbat_test_raw_to_vbat_mv(vbat_test_status.average_raw);

    if ((vbat_test_status.minimum_raw == 0U) ||
        (vbat_test_status.maximum_raw >= BOARD_ADC_12BIT_FULL_SCALE))
    {
        vbat_test_fail(VBAT_TEST_FAILURE_SATURATION);
    }
    else if ((vbat_test_status.average_vbat_mv < BOARD_VBAT_TEST_MIN_MV) ||
             (vbat_test_status.average_vbat_mv > BOARD_VBAT_TEST_MAX_MV))
    {
        vbat_test_fail(VBAT_TEST_FAILURE_VOLTAGE_RANGE);
    }
    else if (vbat_test_status.raw_spread > BOARD_VBAT_TEST_MAX_SPREAD_RAW)
    {
        vbat_test_fail(VBAT_TEST_FAILURE_UNSTABLE);
    }
    else
    {
        vbat_test_status.result = VBAT_TEST_RESULT_PASSED;
        vbat_test_status.phase = VBAT_TEST_PHASE_COMPLETE;
        vbat_test_status.failure_code = VBAT_TEST_FAILURE_NONE;
    }
}

void vbat_test_init(uint32_t current_time_ms)
{
    vbat_test_status.result = VBAT_TEST_RESULT_RUNNING;
    vbat_test_status.phase = VBAT_TEST_PHASE_COLLECT;
    vbat_test_status.failure_code = VBAT_TEST_FAILURE_NONE;
    vbat_test_status.sample_count = 0U;
    vbat_test_status.dma_complete_count = 0U;
    vbat_test_status.adc_error_count = 0U;
    vbat_test_status.timeout_count = 0U;
    vbat_test_status.latest_raw = 0U;
    vbat_test_status.minimum_raw = 0U;
    vbat_test_status.maximum_raw = 0U;
    vbat_test_status.average_raw = 0U;
    vbat_test_status.raw_spread = 0U;
    vbat_test_status.adc_pin_mv = 0U;
    vbat_test_status.latest_vbat_mv = 0U;
    vbat_test_status.minimum_vbat_mv = 0U;
    vbat_test_status.maximum_vbat_mv = 0U;
    vbat_test_status.average_vbat_mv = 0U;

    vbat_test_context.next_sample_time_ms = current_time_ms;
    vbat_test_context.sample_start_time_ms = current_time_ms;
    vbat_test_context.raw_sum = 0U;
    vbat_test_context.sample_active = false;

    if (!bsp_adc_vbat_init())
    {
        vbat_test_fail(VBAT_TEST_FAILURE_INIT);
    }
}

void vbat_test_process(uint32_t current_time_ms)
{
    bsp_adc_vbat_status_t adc_status;
    uint32_t raw;
    uint32_t sequence;

    if (vbat_test_status.phase != VBAT_TEST_PHASE_COLLECT)
    {
        return;
    }

    bsp_adc_vbat_process();
    if (!bsp_adc_vbat_get_status(&adc_status))
    {
        vbat_test_fail(VBAT_TEST_FAILURE_ADC);
        return;
    }
    vbat_test_status.dma_complete_count = adc_status.dma_complete_count;
    vbat_test_status.adc_error_count = adc_status.error_count;
    if (adc_status.error_count != 0U)
    {
        vbat_test_fail(VBAT_TEST_FAILURE_ADC);
        return;
    }

    if (bsp_adc_vbat_read_sample(&raw, &sequence))
    {
        (void)sequence;
        vbat_test_context.sample_active = false;
        vbat_test_update_sample(raw);
        if (vbat_test_status.sample_count == BOARD_VBAT_TEST_SAMPLE_COUNT)
        {
            vbat_test_finish();
            return;
        }
        vbat_test_context.next_sample_time_ms =
            current_time_ms + BOARD_VBAT_SAMPLE_INTERVAL_MS;
    }

    if (vbat_test_context.sample_active &&
        ((current_time_ms - vbat_test_context.sample_start_time_ms) >=
         BOARD_VBAT_SAMPLE_TIMEOUT_MS))
    {
        vbat_test_status.timeout_count++;
        vbat_test_fail(VBAT_TEST_FAILURE_TIMEOUT);
        return;
    }

    if (!vbat_test_context.sample_active &&
        ((int32_t)(current_time_ms - vbat_test_context.next_sample_time_ms) >= 0))
    {
        if (!bsp_adc_vbat_start_sample())
        {
            vbat_test_fail(VBAT_TEST_FAILURE_START);
        }
        else
        {
            vbat_test_context.sample_active = true;
            vbat_test_context.sample_start_time_ms = current_time_ms;
        }
    }
}

bool vbat_test_get_summary(vbat_test_summary_t *summary)
{
    if (summary == NULL)
    {
        return false;
    }

    *summary = vbat_test_status;
    return true;
}
