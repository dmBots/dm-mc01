/*
 * 测量开发板电池输入电压（VBAT），检查电压数值及多次测量是否稳定。
 * ADC 把电压变成数字；程序再根据板上分压电阻，把数字换算回电池端电压。
 * 测完规定次数后，检查读数是否顶到量程边界、平均电压是否合格、最大最小读数是否差得太多。
 * 电压范围和采样数量等设置在 board_config.h；改参数前先看该文件的单位说明。
 * init 启动测试，主循环反复调用 process(HAL_GetTick())，get_summary 查看当前进度。
 */
#ifndef VBAT_TEST_H
#define VBAT_TEST_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    VBAT_TEST_RESULT_IDLE = 0U,
    VBAT_TEST_RESULT_RUNNING,
    VBAT_TEST_RESULT_PASSED,
    VBAT_TEST_RESULT_FAILED
} vbat_test_result_t;

typedef enum
{
    VBAT_TEST_FAILURE_NONE = 0U,
    VBAT_TEST_FAILURE_INIT,
    VBAT_TEST_FAILURE_START,
    VBAT_TEST_FAILURE_TIMEOUT,
    VBAT_TEST_FAILURE_ADC,
    VBAT_TEST_FAILURE_SATURATION,
    VBAT_TEST_FAILURE_VOLTAGE_RANGE,
    VBAT_TEST_FAILURE_UNSTABLE
} vbat_test_failure_t;

typedef struct
{
    uint32_t result;
    uint32_t phase;
    uint32_t failure_code;
    uint32_t sample_count;
    uint32_t dma_complete_count;
    uint32_t adc_error_count;
    uint32_t timeout_count;
    uint32_t latest_raw;
    uint32_t minimum_raw;
    uint32_t maximum_raw;
    uint32_t average_raw;
    uint32_t raw_spread; /* 本轮最大读数减最小读数；它表示波动大小，不是电压值。 */
    uint32_t adc_pin_mv;
    uint32_t latest_vbat_mv; /* 电池端电压，单位 mV（毫伏）；例如 24000 表示 24 V。 */
    uint32_t minimum_vbat_mv;
    uint32_t maximum_vbat_mv;
    uint32_t average_vbat_mv;
} vbat_test_summary_t;

void vbat_test_init(uint32_t current_time_ms);
void vbat_test_process(uint32_t current_time_ms);
/* 返回 true 只表示成功读取了状态；测试是否通过要看 summary->result，不能只看返回值。 */
bool vbat_test_get_summary(vbat_test_summary_t *summary);

#endif
