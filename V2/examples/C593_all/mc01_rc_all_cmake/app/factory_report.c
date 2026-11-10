#include "factory_report.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "beep_music_test.h"
#include "board_config.h"
#include "bsp_usb_cdc.h"
#include "can_test.h"
#include "imu_heater.h"
#include "imu_test.h"
#include "key_test.h"
#include "led_mode_test.h"
#include "rs485_echo_test.h"
#include "tim8_loopback_test.h"
#include "uart_loopback_test.h"
#include "vbat_test.h"

#define FACTORY_REPORT_TX_LINE_SIZE          (64U)
#define FACTORY_REPORT_RX_CHUNK_SIZE         (64U)
#define FACTORY_REPORT_COMMAND_SIZE          (16U)
#define FACTORY_REPORT_STATUS_INTERVAL_MS    (10U)

typedef enum
{
    FACTORY_ITEM_CAN = 0U,
    FACTORY_ITEM_RS485,
    FACTORY_ITEM_UART,
    FACTORY_ITEM_VBAT,
    FACTORY_ITEM_KEY_ADC,
    FACTORY_ITEM_IMU,
    FACTORY_ITEM_HEATER,
    FACTORY_ITEM_TIM8,
    FACTORY_ITEM_BEEP,
    FACTORY_ITEM_LED,
    FACTORY_ITEM_COUNT
} factory_item_t;

typedef enum
{
    FACTORY_ITEM_STATE_NOT_RUN = 0U,
    FACTORY_ITEM_STATE_RUNNING,
    FACTORY_ITEM_STATE_PASSED,
    FACTORY_ITEM_STATE_FAILED,
    FACTORY_ITEM_STATE_BLOCKED
} factory_item_state_t;

typedef enum
{
    FACTORY_OVERALL_RUNNING = 0U,
    FACTORY_OVERALL_PASSED,
    FACTORY_OVERALL_FAILED
} factory_overall_t;

typedef struct
{
    can_test_summary_t can;
    rs485_echo_test_summary_t rs485;
    uart_loopback_test_summary_t uart;
    vbat_test_summary_t vbat;
    key_test_summary_t key;
    imu_test_summary_t imu;
    imu_heater_summary_t heater;
    tim8_loopback_test_summary_t tim8;
    beep_music_test_summary_t beep;
    led_mode_test_summary_t led;
    factory_item_state_t item_states[FACTORY_ITEM_COUNT];
    factory_overall_t overall;
    uint32_t elapsed_ms;
    uint32_t passed_count;
    uint32_t failed_count;
    uint32_t running_count;
    bool timed_out;
} factory_report_snapshot_t;

typedef struct
{
    uint32_t start_time_ms;
    uint32_t last_status_time_ms;
    uint32_t report_sequence;
    uint32_t report_line;
    uint32_t command_length;
    factory_report_snapshot_t final_snapshot;
    factory_report_snapshot_t active_snapshot;
    bool final_snapshot_valid;
    bool automatic_report_pending;
    bool command_report_pending;
    bool report_active;
    bool report_waiting_for_tx;
    bool active_delivers_final;
    bool command_discard;
    bool port_was_open;
} factory_report_context_t;

static factory_report_context_t factory_report_context;
static factory_report_snapshot_t factory_report_poll_snapshot;
static uint8_t factory_report_rx_chunk[FACTORY_REPORT_RX_CHUNK_SIZE];
static char factory_report_command[FACTORY_REPORT_COMMAND_SIZE];
static char factory_report_tx_line[FACTORY_REPORT_TX_LINE_SIZE];

static factory_item_state_t factory_report_standard_state(uint32_t result,
                                                          uint32_t idle,
                                                          uint32_t running,
                                                          uint32_t passed,
                                                          uint32_t failed)
{
    if (result == passed)
    {
        return FACTORY_ITEM_STATE_PASSED;
    }
    if (result == failed)
    {
        return FACTORY_ITEM_STATE_FAILED;
    }
    if (result == running)
    {
        return FACTORY_ITEM_STATE_RUNNING;
    }
    if (result == idle)
    {
        return FACTORY_ITEM_STATE_NOT_RUN;
    }
    return FACTORY_ITEM_STATE_FAILED;
}

static const char *factory_report_item_state_text(factory_item_state_t state)
{
    switch (state)
    {
        case FACTORY_ITEM_STATE_PASSED:
            return "PASS";
        case FACTORY_ITEM_STATE_FAILED:
            return "FAIL";
        case FACTORY_ITEM_STATE_BLOCKED:
            return "BLOCKED";
        case FACTORY_ITEM_STATE_RUNNING:
            return "RUN";
        case FACTORY_ITEM_STATE_NOT_RUN:
        default:
            return "NOT_RUN";
    }
}

static const char *factory_report_overall_text(const factory_report_snapshot_t *snapshot)
{
    if (snapshot->overall == FACTORY_OVERALL_PASSED)
    {
        return "PASS";
    }
    if (snapshot->overall == FACTORY_OVERALL_FAILED)
    {
        return "FAIL";
    }
    return "RUNNING";
}

static void factory_report_capture(factory_report_snapshot_t *snapshot,
                                   uint32_t current_time_ms)
{
    uint32_t item;

    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->elapsed_ms = current_time_ms - factory_report_context.start_time_ms;

    if (!can_test_get_summary(&snapshot->can))
    {
        snapshot->can.result = UINT32_MAX;
    }
    if (!rs485_echo_test_get_summary(&snapshot->rs485))
    {
        snapshot->rs485.result = UINT32_MAX;
    }
    if (!uart_loopback_test_get_summary(&snapshot->uart))
    {
        snapshot->uart.result = UINT32_MAX;
    }
    if (!vbat_test_get_summary(&snapshot->vbat))
    {
        snapshot->vbat.result = UINT32_MAX;
    }
    if (!key_test_get_summary(&snapshot->key))
    {
        snapshot->key.result = UINT32_MAX;
    }
    if (!imu_test_get_summary(&snapshot->imu))
    {
        snapshot->imu.result = UINT32_MAX;
    }
    if (!imu_heater_get_summary(&snapshot->heater))
    {
        snapshot->heater.result = UINT32_MAX;
    }
    if (!tim8_loopback_test_get_summary(&snapshot->tim8))
    {
        snapshot->tim8.result = UINT32_MAX;
    }
    if (!beep_music_test_get_summary(&snapshot->beep))
    {
        snapshot->beep.result = UINT32_MAX;
    }
    if (!led_mode_test_get_summary(&snapshot->led))
    {
        snapshot->led.result = UINT32_MAX;
    }

    snapshot->item_states[FACTORY_ITEM_CAN] =
        factory_report_standard_state(snapshot->can.result,
                                      CAN_TEST_RESULT_IDLE,
                                      CAN_TEST_RESULT_RUNNING,
                                      CAN_TEST_RESULT_PASSED,
                                      CAN_TEST_RESULT_FAILED);
    snapshot->item_states[FACTORY_ITEM_RS485] =
        factory_report_standard_state(snapshot->rs485.result,
                                      RS485_TEST_RESULT_IDLE,
                                      RS485_TEST_RESULT_RUNNING,
                                      RS485_TEST_RESULT_PASSED,
                                      RS485_TEST_RESULT_FAILED);
    snapshot->item_states[FACTORY_ITEM_UART] =
        factory_report_standard_state(snapshot->uart.result,
                                      UART_TEST_RESULT_IDLE,
                                      UART_TEST_RESULT_RUNNING,
                                      UART_TEST_RESULT_PASSED,
                                      UART_TEST_RESULT_FAILED);
    snapshot->item_states[FACTORY_ITEM_VBAT] =
        factory_report_standard_state(snapshot->vbat.result,
                                      VBAT_TEST_RESULT_IDLE,
                                      VBAT_TEST_RESULT_RUNNING,
                                      VBAT_TEST_RESULT_PASSED,
                                      VBAT_TEST_RESULT_FAILED);
    snapshot->item_states[FACTORY_ITEM_KEY_ADC] =
        factory_report_standard_state(snapshot->key.result,
                                      KEY_TEST_RESULT_IDLE,
                                      KEY_TEST_RESULT_RUNNING,
                                      KEY_TEST_RESULT_PASSED,
                                      KEY_TEST_RESULT_FAILED);
    snapshot->item_states[FACTORY_ITEM_IMU] =
        factory_report_standard_state(snapshot->imu.result,
                                      IMU_TEST_RESULT_IDLE,
                                      IMU_TEST_RESULT_RUNNING,
                                      IMU_TEST_RESULT_PASSED,
                                      IMU_TEST_RESULT_FAILED);
    snapshot->item_states[FACTORY_ITEM_HEATER] =
        factory_report_standard_state(snapshot->heater.result,
                                      IMU_HEATER_RESULT_IDLE,
                                      IMU_HEATER_RESULT_RUNNING,
                                      IMU_HEATER_RESULT_PASSED,
                                      IMU_HEATER_RESULT_FAILED);
    snapshot->item_states[FACTORY_ITEM_TIM8] =
        factory_report_standard_state(snapshot->tim8.result,
                                      TIM8_LOOPBACK_RESULT_IDLE,
                                      TIM8_LOOPBACK_RESULT_RUNNING,
                                      TIM8_LOOPBACK_RESULT_PASSED,
                                      TIM8_LOOPBACK_RESULT_FAILED);
    snapshot->item_states[FACTORY_ITEM_BEEP] =
        factory_report_standard_state(snapshot->beep.result,
                                      BEEP_MUSIC_TEST_IDLE,
                                      BEEP_MUSIC_TEST_RUNNING,
                                      BEEP_MUSIC_TEST_PASSED,
                                      BEEP_MUSIC_TEST_FAILED);

    /* 程序切换过一次灯的亮灭就算自动项通过；有没有真的亮、按键能否切速度，还要人来观察。 */
    if (snapshot->led.result == LED_MODE_TEST_RESULT_INIT_ERROR)
    {
        snapshot->item_states[FACTORY_ITEM_LED] = FACTORY_ITEM_STATE_FAILED;
    }
    else if (snapshot->led.result == LED_MODE_TEST_RESULT_RUNNING)
    {
        snapshot->item_states[FACTORY_ITEM_LED] =
            (snapshot->led.led_toggle_count > 0U) ?
            FACTORY_ITEM_STATE_PASSED : FACTORY_ITEM_STATE_RUNNING;
    }
    else if (snapshot->led.result == LED_MODE_TEST_RESULT_NOT_RUN)
    {
        snapshot->item_states[FACTORY_ITEM_LED] = FACTORY_ITEM_STATE_NOT_RUN;
    }
    else
    {
        snapshot->item_states[FACTORY_ITEM_LED] = FACTORY_ITEM_STATE_FAILED;
    }

    /* 运动采样要先等加热稳定；加热已失败却还在等采样时，显示 BLOCKED，说明前面的条件没满足。 */
    if ((snapshot->item_states[FACTORY_ITEM_HEATER] == FACTORY_ITEM_STATE_FAILED) &&
        (snapshot->item_states[FACTORY_ITEM_IMU] == FACTORY_ITEM_STATE_RUNNING))
    {
        snapshot->item_states[FACTORY_ITEM_IMU] = FACTORY_ITEM_STATE_BLOCKED;
    }

    for (item = 0U; item < FACTORY_ITEM_COUNT; item++)
    {
        if (snapshot->item_states[item] == FACTORY_ITEM_STATE_PASSED)
        {
            snapshot->passed_count++;
        }
        else if ((snapshot->item_states[item] == FACTORY_ITEM_STATE_FAILED) ||
                 (snapshot->item_states[item] == FACTORY_ITEM_STATE_BLOCKED))
        {
            snapshot->failed_count++;
        }
        else
        {
            snapshot->running_count++;
        }
    }

    snapshot->timed_out =
        (snapshot->running_count > 0U) &&
        (snapshot->elapsed_ms >= BOARD_FACTORY_TEST_TIMEOUT_MS);
    if (snapshot->timed_out)
    {
        for (item = 0U; item < FACTORY_ITEM_COUNT; item++)
        {
            if ((snapshot->item_states[item] != FACTORY_ITEM_STATE_PASSED) &&
                (snapshot->item_states[item] != FACTORY_ITEM_STATE_FAILED) &&
                (snapshot->item_states[item] != FACTORY_ITEM_STATE_BLOCKED))
            {
                snapshot->item_states[item] =
                    ((item == FACTORY_ITEM_IMU) &&
                     (snapshot->item_states[FACTORY_ITEM_HEATER] !=
                      FACTORY_ITEM_STATE_PASSED)) ?
                    FACTORY_ITEM_STATE_BLOCKED : FACTORY_ITEM_STATE_FAILED;
                snapshot->running_count--;
                snapshot->failed_count++;
            }
        }
        snapshot->overall = FACTORY_OVERALL_FAILED;
    }
    else if (snapshot->running_count > 0U)
    {
        snapshot->overall = FACTORY_OVERALL_RUNNING;
    }
    else if (snapshot->failed_count > 0U)
    {
        snapshot->overall = FACTORY_OVERALL_FAILED;
    }
    else
    {
        snapshot->overall = FACTORY_OVERALL_PASSED;
    }
}

/* 留一份本轮最终结果，之后再发 REPORT 只会重发这一份，不会把测试重新做一遍。 */
static void factory_report_update_final(uint32_t current_time_ms)
{
    if (factory_report_context.final_snapshot_valid ||
        ((current_time_ms - factory_report_context.last_status_time_ms) <
         FACTORY_REPORT_STATUS_INTERVAL_MS))
    {
        return;
    }

    factory_report_context.last_status_time_ms = current_time_ms;
    factory_report_capture(&factory_report_poll_snapshot, current_time_ms);
    if (factory_report_poll_snapshot.overall != FACTORY_OVERALL_RUNNING)
    {
        memcpy(&factory_report_context.final_snapshot,
               &factory_report_poll_snapshot,
               sizeof(factory_report_context.final_snapshot));
        factory_report_context.final_snapshot_valid = true;
        factory_report_context.automatic_report_pending = true;
    }
}

static bool factory_report_command_matches(const char *expected)
{
    uint32_t index = 0U;

    while ((expected[index] != '\0') &&
           (index < factory_report_context.command_length))
    {
        char actual = factory_report_command[index];

        if ((actual >= 'a') && (actual <= 'z'))
        {
            actual = (char)(actual - ('a' - 'A'));
        }
        if (actual != expected[index])
        {
            return false;
        }
        index++;
    }

    return (expected[index] == '\0') &&
           (index == factory_report_context.command_length);
}

static void factory_report_finish_command(void)
{
    if (!factory_report_context.command_discard &&
        (factory_report_context.command_length > 0U) &&
        (factory_report_command_matches("REPORT") ||
         factory_report_command_matches("R")))
    {
        factory_report_context.command_report_pending = true;
    }

    factory_report_context.command_length = 0U;
    factory_report_context.command_discard = false;
}

static void factory_report_receive_commands(void)
{
    uint32_t received_length;
    uint32_t index;

    if (bsp_usb_cdc_read(factory_report_rx_chunk,
                         sizeof(factory_report_rx_chunk),
                         &received_length) != BSP_USB_CDC_STATUS_OK)
    {
        return;
    }

    for (index = 0U; index < received_length; index++)
    {
        uint8_t data = factory_report_rx_chunk[index];

        if ((data == '\r') || (data == '\n'))
        {
            factory_report_finish_command();
        }
        else if (!factory_report_context.command_discard)
        {
            if (factory_report_context.command_length <
                (FACTORY_REPORT_COMMAND_SIZE - 1U))
            {
                factory_report_command[factory_report_context.command_length] =
                    (char)data;
                factory_report_context.command_length++;
            }
            else
            {
                factory_report_context.command_discard = true;
            }
        }
    }
}

static uint32_t factory_report_error_sum_can(const can_test_summary_t *summary)
{
    return summary->can1_tx_error_count + summary->can1_rx_error_count +
           summary->can1_rx_dropped_count + summary->can2_tx_error_count +
           summary->can2_rx_error_count + summary->can2_rx_dropped_count;
}

static uint32_t factory_report_error_sum_rs485(const rs485_echo_test_summary_t *summary)
{
    return summary->port1_error_count + summary->port1_dropped_count +
           summary->port2_error_count + summary->port2_dropped_count;
}

static uint32_t factory_report_error_sum_uart(const uart_loopback_test_summary_t *summary)
{
    return summary->usart1_error_count + summary->usart1_dropped_count +
           summary->usart3_error_count + summary->usart3_dropped_count;
}

static int factory_report_format_item(const factory_report_snapshot_t *snapshot,
                                      factory_item_t item)
{
    const char *state = factory_report_item_state_text(snapshot->item_states[item]);

    switch (item)
    {
        case FACTORY_ITEM_CAN:
            return snprintf(factory_report_tx_line, sizeof(factory_report_tx_line),
                            "@T,CAN,%s,M=%02lX,F=%lu,E=%lu\r\n", state,
                            (unsigned long)snapshot->can.passed_mask,
                            (unsigned long)snapshot->can.failure_code,
                            (unsigned long)factory_report_error_sum_can(&snapshot->can));

        case FACTORY_ITEM_RS485:
            return snprintf(factory_report_tx_line, sizeof(factory_report_tx_line),
                            "@T,RS485,%s,M=%02lX,F=%lu,E=%lu\r\n", state,
                            (unsigned long)snapshot->rs485.passed_mask,
                            (unsigned long)snapshot->rs485.failure_code,
                            (unsigned long)factory_report_error_sum_rs485(&snapshot->rs485));

        case FACTORY_ITEM_UART:
            return snprintf(factory_report_tx_line, sizeof(factory_report_tx_line),
                            "@T,UART,%s,M=%02lX,F=%lu,E=%lu\r\n", state,
                            (unsigned long)snapshot->uart.passed_mask,
                            (unsigned long)snapshot->uart.failure_code,
                            (unsigned long)factory_report_error_sum_uart(&snapshot->uart));

        case FACTORY_ITEM_VBAT:
            return snprintf(factory_report_tx_line, sizeof(factory_report_tx_line),
                            "@T,VBAT,%s,MV=%lu,SP=%lu,F=%lu\r\n", state,
                            (unsigned long)snapshot->vbat.average_vbat_mv,
                            (unsigned long)snapshot->vbat.raw_spread,
                            (unsigned long)snapshot->vbat.failure_code);

        case FACTORY_ITEM_KEY_ADC:
            return snprintf(factory_report_tx_line, sizeof(factory_report_tx_line),
                            "@T,KEY_ADC,%s,RAW=%lu,MIN=%lu,MAX=%lu,F=%lu\r\n", state,
                            (unsigned long)snapshot->key.latest_raw,
                            (unsigned long)snapshot->key.minimum_raw,
                            (unsigned long)snapshot->key.maximum_raw,
                            (unsigned long)snapshot->key.failure_code);

        case FACTORY_ITEM_IMU:
            return snprintf(factory_report_tx_line, sizeof(factory_report_tx_line),
                            "@T,IMU,%s,A=%02lX,G=%02lX,N=%lu,F=%lu,E=%lu\r\n", state,
                            (unsigned long)snapshot->imu.accel_chip_id,
                            (unsigned long)snapshot->imu.gyro_chip_id,
                            (unsigned long)snapshot->imu.sample_count,
                            (unsigned long)snapshot->imu.failure_step,
                            (unsigned long)snapshot->imu.read_error_count);

        case FACTORY_ITEM_HEATER:
            return snprintf(factory_report_tx_line, sizeof(factory_report_tx_line),
                            "@T,HEAT,%s,T=%ldmC,D=%lu,F=%lu\r\n", state,
                            (long)snapshot->heater.temperature_milli_c,
                            (unsigned long)snapshot->heater.duty_per_mille,
                            (unsigned long)snapshot->heater.fault);

        case FACTORY_ITEM_TIM8:
            return snprintf(factory_report_tx_line, sizeof(factory_report_tx_line),
                            "@T,TIM8,%s,M=%lX,C2=%lu,C4=%lu\r\n", state,
                            (unsigned long)snapshot->tim8.failure_mask,
                            (unsigned long)snapshot->tim8.ch2_edge_count,
                            (unsigned long)snapshot->tim8.ch4_edge_count);

        case FACTORY_ITEM_BEEP:
            return snprintf(factory_report_tx_line, sizeof(factory_report_tx_line),
                            "@T,BEEP,%s,N=%lu/%lu\r\n", state,
                            (unsigned long)snapshot->beep.note_index,
                            (unsigned long)snapshot->beep.note_count);

        case FACTORY_ITEM_LED:
            return snprintf(factory_report_tx_line, sizeof(factory_report_tx_line),
                            "@T,LED,%s,MODE=%lu,P=%lu,T=%lu\r\n", state,
                            (unsigned long)snapshot->led.mode,
                            (unsigned long)snapshot->led.accepted_press_count,
                            (unsigned long)snapshot->led.led_toggle_count);

        case FACTORY_ITEM_COUNT:
        default:
            return -1;
    }
}

/* 先发开头和各项进度，再发人工检查提示与汇总，最后用中文说明结果；只看末行也能知道结论。 */
static uint32_t factory_report_format_line(void)
{
    int length;

    if (factory_report_context.report_line == 0U)
    {
        length = snprintf(factory_report_tx_line, sizeof(factory_report_tx_line),
                          "@BEGIN,V=1,S=%lu,K=%s,MS=%lu\r\n",
                          (unsigned long)factory_report_context.report_sequence,
                          factory_report_context.active_delivers_final ? "FINAL" : "LIVE",
                          (unsigned long)factory_report_context.active_snapshot.elapsed_ms);
    }
    else if (factory_report_context.report_line <= FACTORY_ITEM_COUNT)
    {
        length = factory_report_format_item(
            &factory_report_context.active_snapshot,
            (factory_item_t)(factory_report_context.report_line - 1U));
    }
    else if (factory_report_context.report_line == (FACTORY_ITEM_COUNT + 1U))
    {
        length = snprintf(factory_report_tx_line, sizeof(factory_report_tx_line),
                          "@MANUAL,LED=CHECK,BEEP=CHECK,KEY_POS=CHECK\r\n");
    }
    else if (factory_report_context.report_line == (FACTORY_ITEM_COUNT + 2U))
    {
        length = snprintf(factory_report_tx_line, sizeof(factory_report_tx_line),
                          "@END,S=%lu,AUTO=%s,P=%lu,F=%lu,R=%lu,TO=%lu\r\n",
                          (unsigned long)factory_report_context.report_sequence,
                          factory_report_overall_text(&factory_report_context.active_snapshot),
                          (unsigned long)factory_report_context.active_snapshot.passed_count,
                          (unsigned long)factory_report_context.active_snapshot.failed_count,
                          (unsigned long)factory_report_context.active_snapshot.running_count,
                          factory_report_context.active_snapshot.timed_out ? 1UL : 0UL);
    }
    else if (factory_report_context.active_snapshot.timed_out)
    {
        length = snprintf(factory_report_tx_line, sizeof(factory_report_tx_line),
                          "最终结果:测试超时;查看上方FAIL项\r\n");
    }
    else if (factory_report_context.active_snapshot.overall == FACTORY_OVERALL_FAILED)
    {
        length = snprintf(factory_report_tx_line, sizeof(factory_report_tx_line),
                          "最终结果:自动失败;查看上方FAIL项\r\n");
    }
    else if (factory_report_context.active_snapshot.overall == FACTORY_OVERALL_PASSED)
    {
        length = snprintf(factory_report_tx_line, sizeof(factory_report_tx_line),
                          "最终结果:自动通过;人工检查灯/蜂鸣器/按键\r\n");
    }
    else
    {
        length = snprintf(factory_report_tx_line, sizeof(factory_report_tx_line),
                          "当前状态:测试进行中;请等待最终结果\r\n");
    }

    if ((length <= 0) || ((uint32_t)length >= sizeof(factory_report_tx_line)))
    {
        static const char format_error[] = "@ERROR,REPORT_FORMAT\r\n";

        memcpy(factory_report_tx_line, format_error, sizeof(format_error));
        return sizeof(format_error) - 1U;
    }
    return (uint32_t)length;
}

static void factory_report_start(uint32_t current_time_ms)
{
    if (factory_report_context.final_snapshot_valid)
    {
        factory_report_context.active_snapshot = factory_report_context.final_snapshot;
        factory_report_context.active_delivers_final = true;
    }
    else
    {
        factory_report_capture(&factory_report_context.active_snapshot, current_time_ms);
        factory_report_context.active_delivers_final = false;
    }

    factory_report_context.report_sequence++;
    factory_report_context.report_line = 0U;
    factory_report_context.report_active = true;
    factory_report_context.report_waiting_for_tx = false;
    factory_report_context.command_report_pending = false;
}

static void factory_report_abort(void)
{
    if (factory_report_context.active_delivers_final)
    {
        factory_report_context.automatic_report_pending = true;
    }
    factory_report_context.report_active = false;
    factory_report_context.report_waiting_for_tx = false;
}

static void factory_report_send(uint32_t current_time_ms)
{
    bsp_usb_cdc_tx_completion_t completion;
    bsp_usb_cdc_status_t status;
    uint32_t length;

    if (!factory_report_context.report_active)
    {
        if (factory_report_context.command_report_pending ||
            (factory_report_context.automatic_report_pending &&
             bsp_usb_cdc_is_port_open()))
        {
            factory_report_start(current_time_ms);
        }
        return;
    }

    /* 上一行确认发完才处理下一行；USB 还在发送时先返回，稍后由主循环再来检查。 */
    if (factory_report_context.report_waiting_for_tx)
    {
        completion = bsp_usb_cdc_take_tx_completion();
        if (completion == BSP_USB_CDC_TX_COMPLETION_SUCCESS)
        {
            factory_report_context.report_waiting_for_tx = false;
            if (factory_report_context.report_line == (FACTORY_ITEM_COUNT + 3U))
            {
                factory_report_context.report_active = false;
                if (factory_report_context.active_delivers_final)
                {
                    factory_report_context.automatic_report_pending = false;
                }
            }
            else
            {
                factory_report_context.report_line++;
            }
        }
        else if ((completion == BSP_USB_CDC_TX_COMPLETION_FAILED) ||
                 !bsp_usb_cdc_is_connected())
        {
            factory_report_abort();
        }
        return;
    }

    if (!bsp_usb_cdc_is_connected())
    {
        factory_report_abort();
        return;
    }

    if (bsp_usb_cdc_tx_busy())
    {
        return;
    }

    length = factory_report_format_line();
    status = bsp_usb_cdc_write((const uint8_t *)factory_report_tx_line, length);
    if (status == BSP_USB_CDC_STATUS_OK)
    {
        factory_report_context.report_waiting_for_tx = true;
    }
    else if (status != BSP_USB_CDC_STATUS_BUSY)
    {
        factory_report_abort();
    }
}

void factory_report_init(uint32_t current_time_ms)
{
    memset(&factory_report_context, 0, sizeof(factory_report_context));
    memset(&factory_report_poll_snapshot, 0, sizeof(factory_report_poll_snapshot));
    factory_report_context.start_time_ms = current_time_ms;
    factory_report_context.last_status_time_ms = current_time_ms;
}

void factory_report_process(uint32_t current_time_ms)
{
    bool port_open;

    factory_report_update_final(current_time_ms);
    factory_report_receive_commands();

    port_open = bsp_usb_cdc_is_port_open();
    if (port_open && !factory_report_context.port_was_open)
    {
        factory_report_context.command_report_pending = true;
    }
    factory_report_context.port_was_open = port_open;

    factory_report_send(current_time_ms);
}
