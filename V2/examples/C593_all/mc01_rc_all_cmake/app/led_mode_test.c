#include "led_mode_test.h"

#include "board_config.h"
#include "bsp_led.h"
#include "bsp_user_key.h"

#include <stdbool.h>
#include <stddef.h>

volatile led_mode_test_summary_t led_mode_test_status;

static uint32_t observed_interrupt_count;
static uint32_t debounce_start_time_ms;
static bool stable_pressed;
static bool candidate_pressed;
static bool debounce_active;

static uint32_t led_mode_test_get_interval(led_mode_test_mode_t mode)
{
    static const uint32_t intervals[LED_MODE_TEST_COUNT] = {
        BOARD_USER_LED_SLOW_INTERVAL_MS,
        BOARD_USER_LED_NORMAL_INTERVAL_MS,
        BOARD_USER_LED_FAST_INTERVAL_MS,
    };

    return intervals[mode];
}

static void led_mode_test_select(led_mode_test_mode_t mode,
                                 uint32_t current_time_ms)
{
    led_mode_test_status.mode = (uint32_t)mode;
    led_mode_test_status.interval_ms = led_mode_test_get_interval(mode);
    led_mode_test_status.last_toggle_time_ms = current_time_ms;
    led_mode_test_status.led_on = 1U;
    bsp_led_set_state(true);
}

void led_mode_test_init(uint32_t current_time_ms)
{
    led_mode_test_status.magic = 0x4C45444DU;
    led_mode_test_status.result = LED_MODE_TEST_RESULT_NOT_RUN;
    led_mode_test_status.key_interrupt_count = 0U;
    led_mode_test_status.accepted_press_count = 0U;
    led_mode_test_status.debounce_reject_count = 0U;
    led_mode_test_status.led_toggle_count = 0U;
    led_mode_test_status.last_press_time_ms = 0U;
    observed_interrupt_count = 0U;
    debounce_active = false;

    if (!bsp_user_key_init())
    {
        led_mode_test_status.result = LED_MODE_TEST_RESULT_INIT_ERROR;
        bsp_led_set_state(false);
        return;
    }

    stable_pressed = bsp_user_key_is_pressed();
    led_mode_test_select(LED_MODE_TEST_NORMAL, current_time_ms);
    led_mode_test_status.result = LED_MODE_TEST_RESULT_RUNNING;
}

void led_mode_test_process(uint32_t current_time_ms)
{
    bsp_user_key_status_t key_status;

    if (led_mode_test_status.result != LED_MODE_TEST_RESULT_RUNNING)
    {
        return;
    }

    bsp_user_key_get_status(&key_status);
    led_mode_test_status.key_interrupt_count = key_status.interrupt_count;

    if (key_status.interrupt_count != observed_interrupt_count)
    {
        observed_interrupt_count = key_status.interrupt_count;
        candidate_pressed = key_status.pressed;
        debounce_start_time_ms = current_time_ms;
        debounce_active = true;
    }

    /* 等按键信号稳定后才确认按下，避免按一次产生多次通知、让灯连续跳过好几档。 */
    if (debounce_active &&
        ((current_time_ms - debounce_start_time_ms) >= BOARD_USER_KEY_DEBOUNCE_MS))
    {
        bool pressed = bsp_user_key_is_pressed();

        debounce_active = false;
        if (pressed != candidate_pressed)
        {
            led_mode_test_status.debounce_reject_count++;
        }
        else if (pressed != stable_pressed)
        {
            stable_pressed = pressed;
            if (pressed)
            {
                led_mode_test_mode_t next_mode =
                    (led_mode_test_mode_t)((led_mode_test_status.mode + 1U) %
                                           (uint32_t)LED_MODE_TEST_COUNT);
                led_mode_test_status.accepted_press_count++;
                led_mode_test_status.last_press_time_ms = current_time_ms;
                led_mode_test_select(next_mode, current_time_ms);
            }
        }
    }

    if ((current_time_ms - led_mode_test_status.last_toggle_time_ms) >=
        led_mode_test_status.interval_ms)
    {
        uint32_t elapsed_intervals =
            (current_time_ms - led_mode_test_status.last_toggle_time_ms) /
            led_mode_test_status.interval_ms;

        led_mode_test_status.last_toggle_time_ms +=
            elapsed_intervals * led_mode_test_status.interval_ms;
        if ((elapsed_intervals & 1U) != 0U)
        {
            bsp_led_toggle();
            led_mode_test_status.led_on ^= 1U;
            led_mode_test_status.led_toggle_count++;
        }
    }
}

bool led_mode_test_get_summary(led_mode_test_summary_t *summary)
{
    if (summary == NULL)
    {
        return false;
    }

    *summary = led_mode_test_status;
    return true;
}
