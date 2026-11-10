#include "usb_test.h"

#include <math.h>
#include <string.h>

#include "board_config.h"
#include "bsp_usb_cdc.h"

#define USB_TEST_ECHO_BUFFER_SIZE        (64U)
#define USB_TEST_JUSTFLOAT_FRAME_SIZE    (8U)
#define USB_TEST_TWO_PI                  (6.28318530717958647692F)

#if BOARD_USB_TEST_MODE == BOARD_USB_TEST_ECHO
static uint8_t echo_buffer[USB_TEST_ECHO_BUFFER_SIZE];
static uint32_t echo_length;

static void usb_test_echo_process(void)
{
    bsp_usb_cdc_status_t status;
    uint32_t received_length;

    if (echo_length > 0U)
    {
        if (bsp_usb_cdc_write(echo_buffer, echo_length) == BSP_USB_CDC_STATUS_OK)
        {
            echo_length = 0U;
        }
        return;
    }

    status = bsp_usb_cdc_read(echo_buffer, sizeof(echo_buffer), &received_length);
    if ((status == BSP_USB_CDC_STATUS_OK) && (received_length > 0U))
    {
        echo_length = received_length;
    }
}

#else
static uint32_t last_sample_time_ms;
static float sine_phase;

static void usb_test_vofa_process(uint32_t current_time_ms)
{
    /* 每次发送一个 float 类型数值，再附上四个固定字节作为结束标记，电脑要用 JustFloat 方式读取。 */
    static const uint8_t justfloat_tail[4] = {0x00U, 0x00U, 0x80U, 0x7FU};
    uint8_t frame[USB_TEST_JUSTFLOAT_FRAME_SIZE];
    float sample;
    float phase_step;

    if (((current_time_ms - last_sample_time_ms) < BOARD_USB_VOFA_SAMPLE_INTERVAL_MS) ||
        !bsp_usb_cdc_is_connected() || bsp_usb_cdc_tx_busy())
    {
        return;
    }

    sample = BOARD_USB_VOFA_SINE_AMPLITUDE * sinf(sine_phase);
    memcpy(frame, &sample, sizeof(sample));
    memcpy(&frame[sizeof(sample)], justfloat_tail, sizeof(justfloat_tail));
    if (bsp_usb_cdc_write(frame, sizeof(frame)) != BSP_USB_CDC_STATUS_OK)
    {
        return;
    }

    last_sample_time_ms = current_time_ms;
    phase_step = USB_TEST_TWO_PI * BOARD_USB_VOFA_SINE_FREQUENCY_HZ *
                 ((float)BOARD_USB_VOFA_SAMPLE_INTERVAL_MS / 1000.0F);
    sine_phase += phase_step;
    if (sine_phase >= USB_TEST_TWO_PI)
    {
        sine_phase -= USB_TEST_TWO_PI;
    }
}
#endif

void usb_test_init(uint32_t current_time_ms)
{
#if BOARD_USB_TEST_MODE == BOARD_USB_TEST_ECHO
    (void)current_time_ms;
    echo_length = 0U;
#else
    last_sample_time_ms = current_time_ms;
    sine_phase = 0.0F;
#endif
}

void usb_test_process(uint32_t current_time_ms)
{
#if BOARD_USB_TEST_MODE == BOARD_USB_TEST_ECHO
    (void)current_time_ms;
    usb_test_echo_process();
#else
    usb_test_vofa_process(current_time_ms);
#endif
}
