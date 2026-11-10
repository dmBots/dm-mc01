#include "sbus_test.h"

#include <stdbool.h>

#include "board_config.h"
#include "bsp_sbus.h"

#define SBUS_HEADER_BYTE                   (0x0FU)
#define SBUS_FLAG_DIGITAL_CHANNEL_17       (1UL << 0U)
#define SBUS_FLAG_DIGITAL_CHANNEL_18       (1UL << 1U)
#define SBUS_FLAG_FRAME_LOST               (1UL << 2U)
#define SBUS_FLAG_FAILSAFE                 (1UL << 3U)

#define SBUS_TEST_FAILURE_NONE             (0U)
#define SBUS_TEST_FAILURE_PARSER           (1U)
#define SBUS_TEST_FAILURE_START            (2U)
#define SBUS_TEST_FAILURE_NO_DATA          (3U)
#define SBUS_TEST_FAILURE_NO_VALID_FRAME   (4U)
#define SBUS_TEST_FAILURE_UART             (5U)
#define SBUS_TEST_FAILURE_QUEUE            (6U)
#define SBUS_TEST_FAILURE_TOO_FEW_FRAMES   (7U)

typedef enum
{
    SBUS_TEST_PHASE_IDLE = 0U,
    SBUS_TEST_PHASE_WAIT_FRAMES,
    SBUS_TEST_PHASE_COMPLETE,
    SBUS_TEST_PHASE_FAILED
} sbus_test_phase_t;

typedef enum
{
    SBUS_TEST_RESULT_IDLE = 0U,
    SBUS_TEST_RESULT_RUNNING,
    SBUS_TEST_RESULT_PASSED,
    SBUS_TEST_RESULT_FAILED
} sbus_test_result_t;

typedef struct
{
    uint16_t channels[16];
    bool digital_channel_17;
    bool digital_channel_18;
    bool frame_lost;
    bool failsafe;
} sbus_frame_t;

typedef struct
{
    uint32_t baud_rate;
    uint32_t result;
    uint32_t phase;
    uint32_t failure_code;
    uint32_t parser_self_test_passed;
    uint32_t signal_present;
    uint32_t raw_byte_count;
    uint32_t valid_frame_count;
    uint32_t invalid_frame_count;
    uint32_t sync_dropped_byte_count;
    uint32_t lost_frame_count;
    uint32_t failsafe_frame_count;
    uint32_t digital_channel_17;
    uint32_t digital_channel_18;
    uint32_t last_frame_time_ms;
    uint32_t frame_age_ms;
    uint32_t uart_error_count;
    uint32_t uart_dropped_count;
    uint32_t dma_half_event_count;
    uint32_t dma_full_event_count;
    uint32_t dma_restart_count;
    uint32_t channels[16];
} sbus_test_status_t;

typedef struct
{
    uint8_t frame_buffer[BOARD_SBUS_FRAME_SIZE];
    uint32_t frame_position;
    uint32_t start_time_ms;
} sbus_test_context_t;

static volatile sbus_test_status_t sbus_test_status;
static volatile uint8_t sbus_last_raw_frame[BOARD_SBUS_FRAME_SIZE];
static sbus_test_context_t sbus_test_context;

static bool sbus_test_footer_is_valid(uint8_t footer)
{
    return (footer == 0x00U) || ((footer & 0x0FU) == 0x04U);
}

static bool sbus_test_decode_frame(const uint8_t *raw_frame, sbus_frame_t *frame)
{
    uint32_t channel;

    if ((raw_frame[0] != SBUS_HEADER_BYTE) ||
        !sbus_test_footer_is_valid(raw_frame[BOARD_SBUS_FRAME_SIZE - 1U]))
    {
        return false;
    }

    for (channel = 0U; channel < 16U; channel++)
    {
        uint32_t bit_index = channel * 11U;
        uint32_t byte_index = 1U + (bit_index / 8U);
        uint32_t bit_offset = bit_index % 8U;
        uint32_t packed = (uint32_t)raw_frame[byte_index] |
                          ((uint32_t)raw_frame[byte_index + 1U] << 8U) |
                          ((uint32_t)raw_frame[byte_index + 2U] << 16U);

        frame->channels[channel] = (uint16_t)((packed >> bit_offset) & 0x07FFU);
    }

    frame->digital_channel_17 =
        (raw_frame[23] & SBUS_FLAG_DIGITAL_CHANNEL_17) != 0U;
    frame->digital_channel_18 =
        (raw_frame[23] & SBUS_FLAG_DIGITAL_CHANNEL_18) != 0U;
    frame->frame_lost = (raw_frame[23] & SBUS_FLAG_FRAME_LOST) != 0U;
    frame->failsafe = (raw_frame[23] & SBUS_FLAG_FAILSAFE) != 0U;
    return true;
}

static void sbus_test_pack_self_test_frame(uint8_t *raw_frame,
                                          const uint16_t *channels)
{
    uint32_t index;
    uint32_t channel;

    for (index = 0U; index < BOARD_SBUS_FRAME_SIZE; index++)
    {
        raw_frame[index] = 0U;
    }
    raw_frame[0] = SBUS_HEADER_BYTE;

    for (channel = 0U; channel < 16U; channel++)
    {
        uint32_t bit;

        for (bit = 0U; bit < 11U; bit++)
        {
            if ((channels[channel] & (1UL << bit)) != 0U)
            {
                uint32_t bit_index = (channel * 11U) + bit;
                raw_frame[1U + (bit_index / 8U)] |= (uint8_t)(1UL << (bit_index % 8U));
            }
        }
    }
    raw_frame[23] = SBUS_FLAG_DIGITAL_CHANNEL_17 | SBUS_FLAG_DIGITAL_CHANNEL_18;
}

static bool sbus_test_parser_self_test(void)
{
    static const uint16_t expected_channels[16] =
    {
        0U, 1U, 42U, 172U, 500U, 777U, 992U, 1024U,
        1234U, 1500U, 1600U, 1811U, 1900U, 2000U, 2046U, 2047U
    };
    uint8_t raw_frame[BOARD_SBUS_FRAME_SIZE];
    sbus_frame_t frame;
    uint32_t channel;

    sbus_test_pack_self_test_frame(raw_frame, expected_channels);
    if (!sbus_test_decode_frame(raw_frame, &frame) ||
        !frame.digital_channel_17 || !frame.digital_channel_18 ||
        frame.frame_lost || frame.failsafe)
    {
        return false;
    }

    for (channel = 0U; channel < 16U; channel++)
    {
        if (frame.channels[channel] != expected_channels[channel])
        {
            return false;
        }
    }
    return true;
}

static void sbus_test_publish_frame(const sbus_frame_t *frame, uint32_t current_time_ms)
{
    uint32_t channel;
    uint32_t index;

    for (channel = 0U; channel < 16U; channel++)
    {
        sbus_test_status.channels[channel] = frame->channels[channel];
    }
    for (index = 0U; index < BOARD_SBUS_FRAME_SIZE; index++)
    {
        sbus_last_raw_frame[index] = sbus_test_context.frame_buffer[index];
    }

    sbus_test_status.digital_channel_17 = frame->digital_channel_17 ? 1U : 0U;
    sbus_test_status.digital_channel_18 = frame->digital_channel_18 ? 1U : 0U;
    if (frame->frame_lost)
    {
        sbus_test_status.lost_frame_count++;
    }
    if (frame->failsafe)
    {
        sbus_test_status.failsafe_frame_count++;
    }
    sbus_test_status.valid_frame_count++;
    sbus_test_status.last_frame_time_ms = current_time_ms;
    sbus_test_status.frame_age_ms = 0U;
    sbus_test_status.signal_present = 1U;
}

static void sbus_test_resynchronize(void)
{
    uint32_t header_index;

    for (header_index = 1U; header_index < BOARD_SBUS_FRAME_SIZE; header_index++)
    {
        if (sbus_test_context.frame_buffer[header_index] == SBUS_HEADER_BYTE)
        {
            uint32_t remaining = BOARD_SBUS_FRAME_SIZE - header_index;
            uint32_t index;

            for (index = 0U; index < remaining; index++)
            {
                sbus_test_context.frame_buffer[index] =
                    sbus_test_context.frame_buffer[header_index + index];
            }
            sbus_test_context.frame_position = remaining;
            sbus_test_status.sync_dropped_byte_count += header_index;
            return;
        }
    }

    sbus_test_status.sync_dropped_byte_count += BOARD_SBUS_FRAME_SIZE;
    sbus_test_context.frame_position = 0U;
}

static void sbus_test_process_byte(uint8_t data, uint32_t current_time_ms)
{
    sbus_frame_t frame;

    sbus_test_status.raw_byte_count++;
    if (sbus_test_context.frame_position == 0U)
    {
        if (data == SBUS_HEADER_BYTE)
        {
            sbus_test_context.frame_buffer[0] = data;
            sbus_test_context.frame_position = 1U;
        }
        else
        {
            sbus_test_status.sync_dropped_byte_count++;
        }
        return;
    }

    sbus_test_context.frame_buffer[sbus_test_context.frame_position] = data;
    sbus_test_context.frame_position++;
    if (sbus_test_context.frame_position < BOARD_SBUS_FRAME_SIZE)
    {
        return;
    }

    if (sbus_test_decode_frame(sbus_test_context.frame_buffer, &frame))
    {
        sbus_test_publish_frame(&frame, current_time_ms);
        sbus_test_context.frame_position = 0U;
    }
    else
    {
        sbus_test_status.invalid_frame_count++;
        sbus_test_resynchronize();
    }
}

static void sbus_test_capture_bsp_status(void)
{
    bsp_sbus_status_t status;

    if (bsp_sbus_get_status(&status))
    {
        sbus_test_status.uart_error_count = status.error_count;
        sbus_test_status.uart_dropped_count = status.rx_dropped_count;
        sbus_test_status.dma_half_event_count = status.rx_half_event_count;
        sbus_test_status.dma_full_event_count = status.rx_full_event_count;
        sbus_test_status.dma_restart_count = status.restart_count;
    }
}

static uint32_t sbus_test_failure_code(void)
{
    if (sbus_test_status.uart_error_count != 0U)
    {
        return SBUS_TEST_FAILURE_UART;
    }
    if (sbus_test_status.uart_dropped_count != 0U)
    {
        return SBUS_TEST_FAILURE_QUEUE;
    }
    if (sbus_test_status.raw_byte_count == 0U)
    {
        return SBUS_TEST_FAILURE_NO_DATA;
    }
    if (sbus_test_status.valid_frame_count == 0U)
    {
        return SBUS_TEST_FAILURE_NO_VALID_FRAME;
    }
    return SBUS_TEST_FAILURE_TOO_FEW_FRAMES;
}

void sbus_test_init(uint32_t current_time_ms)
{
    uint32_t channel;

    sbus_test_status.baud_rate = BOARD_SBUS_BAUD_RATE;
    sbus_test_status.result = SBUS_TEST_RESULT_RUNNING;
    sbus_test_status.phase = SBUS_TEST_PHASE_WAIT_FRAMES;
    sbus_test_status.failure_code = SBUS_TEST_FAILURE_NONE;
    sbus_test_status.parser_self_test_passed = 0U;
    sbus_test_status.signal_present = 0U;
    sbus_test_status.raw_byte_count = 0U;
    sbus_test_status.valid_frame_count = 0U;
    sbus_test_status.invalid_frame_count = 0U;
    sbus_test_status.sync_dropped_byte_count = 0U;
    sbus_test_status.lost_frame_count = 0U;
    sbus_test_status.failsafe_frame_count = 0U;
    sbus_test_status.digital_channel_17 = 0U;
    sbus_test_status.digital_channel_18 = 0U;
    sbus_test_status.last_frame_time_ms = 0U;
    sbus_test_status.frame_age_ms = 0U;
    sbus_test_status.uart_error_count = 0U;
    sbus_test_status.uart_dropped_count = 0U;
    sbus_test_status.dma_half_event_count = 0U;
    sbus_test_status.dma_full_event_count = 0U;
    sbus_test_status.dma_restart_count = 0U;
    for (channel = 0U; channel < 16U; channel++)
    {
        sbus_test_status.channels[channel] = 0U;
    }

    sbus_test_context.frame_position = 0U;
    sbus_test_context.start_time_ms = current_time_ms;

    /* 先用预置数据检查“字节如何变成通道值”的代码；这一步不检查外部接收机或接线。 */
    if (!sbus_test_parser_self_test())
    {
        sbus_test_status.result = SBUS_TEST_RESULT_FAILED;
        sbus_test_status.phase = SBUS_TEST_PHASE_FAILED;
        sbus_test_status.failure_code = SBUS_TEST_FAILURE_PARSER;
        return;
    }
    sbus_test_status.parser_self_test_passed = 1U;

    if (!bsp_sbus_start())
    {
        sbus_test_status.result = SBUS_TEST_RESULT_FAILED;
        sbus_test_status.phase = SBUS_TEST_PHASE_FAILED;
        sbus_test_status.failure_code = SBUS_TEST_FAILURE_START;
    }
}

void sbus_test_process(uint32_t current_time_ms)
{
    uint8_t received_data[BOARD_SBUS_PROCESS_BUDGET_BYTES];
    uint32_t received_size;
    uint32_t index;

    if (sbus_test_status.parser_self_test_passed == 0U)
    {
        return;
    }

    bsp_sbus_process();
    /* 每轮只取一小批字节，处理完就返回；其他演示仍有机会运行，不一直停在这里收数据。 */
    received_size = bsp_sbus_read(received_data, BOARD_SBUS_PROCESS_BUDGET_BYTES);
    for (index = 0U; index < received_size; index++)
    {
        sbus_test_process_byte(received_data[index], current_time_ms);
    }
    sbus_test_capture_bsp_status();

    if (sbus_test_status.valid_frame_count != 0U)
    {
        sbus_test_status.frame_age_ms = current_time_ms - sbus_test_status.last_frame_time_ms;
        sbus_test_status.signal_present =
            (sbus_test_status.frame_age_ms < BOARD_SBUS_TEST_TIMEOUT_MS) ? 1U : 0U;
    }

    if ((sbus_test_status.valid_frame_count >= BOARD_SBUS_TEST_VALID_FRAME_COUNT) &&
        (sbus_test_status.uart_error_count == 0U) &&
        (sbus_test_status.uart_dropped_count == 0U))
    {
        sbus_test_status.result = SBUS_TEST_RESULT_PASSED;
        sbus_test_status.phase = SBUS_TEST_PHASE_COMPLETE;
        sbus_test_status.failure_code = SBUS_TEST_FAILURE_NONE;
    }
    else if ((current_time_ms - sbus_test_context.start_time_ms) >=
             BOARD_SBUS_TEST_TIMEOUT_MS)
    {
        sbus_test_status.result = SBUS_TEST_RESULT_FAILED;
        sbus_test_status.phase = SBUS_TEST_PHASE_FAILED;
        sbus_test_status.failure_code = sbus_test_failure_code();
    }
}
