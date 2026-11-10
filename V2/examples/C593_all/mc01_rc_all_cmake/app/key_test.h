/*
 * 读取模拟按键对应的电压，并把读数提供给 LCD 显示，用来观察按键操作是否改变数值。
 * 这里的按键不是用来切换 LED 闪烁速度的板载用户按键，两者的程序分开处理。
 * 收集足够多次读数后会显示自动通过，但这只说明程序能够采样，不代表每个按键位置都已检查。
 * 请实际操作模拟按键并观察屏幕上的数值变化；测试通过后，程序仍会继续刷新读数。
 */
#ifndef KEY_TEST_H
#define KEY_TEST_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    KEY_TEST_RESULT_IDLE = 0U,
    KEY_TEST_RESULT_RUNNING,
    KEY_TEST_RESULT_PASSED,
    KEY_TEST_RESULT_FAILED
} key_test_result_t;

typedef enum
{
    KEY_TEST_FAILURE_NONE = 0U,
    KEY_TEST_FAILURE_INITIALIZATION,
    KEY_TEST_FAILURE_START,
    KEY_TEST_FAILURE_TIMEOUT,
    KEY_TEST_FAILURE_ADC
} key_test_failure_t;

typedef struct
{
    uint32_t result;
    uint32_t failure_code;
    uint32_t sample_count;
    uint32_t latest_raw;
    uint32_t minimum_raw;
    uint32_t maximum_raw;
    uint32_t latest_pin_mv;
    uint32_t dma_complete_count;
    uint32_t adc_error_count;
    uint32_t timeout_count;
} key_test_summary_t;

/* init 在开始时调用一次；process 放在主循环中反复调用，时间使用 HAL_GetTick()。 */
void key_test_init(uint32_t current_time_ms);
void key_test_process(uint32_t current_time_ms);
/* 取最近一次读数 raw 和编号 sequence；重复读取不会清除它，尚无读数时返回 false。 */
bool key_test_get_latest(uint32_t *raw, uint32_t *sequence);
/* 返回 true 只表示成功读取了状态；测试是否通过要看 summary->result，不能只看返回值。 */
bool key_test_get_summary(key_test_summary_t *summary);

#endif
