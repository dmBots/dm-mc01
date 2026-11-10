/*
 * 让板载 LED 闪烁，并用板载用户按键切换闪烁速度。
 * 上电后从正常速度开始；依次按下会切到快、慢、正常，然后继续循环。
 * “消抖”是等按键电平稳定后才算一次有效按下，避免按一次被误认为按了多次。
 * init 调用一次，process(HAL_GetTick()) 放在主循环中一直调用，观察灯光并实际按键确认。
 */
#ifndef LED_MODE_TEST_H
#define LED_MODE_TEST_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    LED_MODE_TEST_SLOW = 0,
    LED_MODE_TEST_NORMAL,
    LED_MODE_TEST_FAST,
    LED_MODE_TEST_COUNT
} led_mode_test_mode_t;

typedef enum
{
    LED_MODE_TEST_RESULT_NOT_RUN = 0U,
    LED_MODE_TEST_RESULT_RUNNING,
    LED_MODE_TEST_RESULT_INIT_ERROR
} led_mode_test_result_t;

typedef struct
{
    uint32_t magic;
    uint32_t result;
    uint32_t mode;
    uint32_t interval_ms;
    uint32_t key_interrupt_count;
    uint32_t accepted_press_count;
    uint32_t debounce_reject_count;
    uint32_t led_toggle_count;
    uint32_t led_on;
    uint32_t last_toggle_time_ms;
    uint32_t last_press_time_ms;
} led_mode_test_summary_t;

void led_mode_test_init(uint32_t current_time_ms);
void led_mode_test_process(uint32_t current_time_ms);
bool led_mode_test_get_summary(led_mode_test_summary_t *summary);

#endif
