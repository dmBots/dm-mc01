#include "rs485_echo_test.h"

#include <stdbool.h>

#include "board_config.h"
#include "bsp_rs485.h"

#define RS485_TEST_CASE_COUNT   (6U)

typedef enum
{
    RS485_TEST_PHASE_IDLE = 0U,
    RS485_TEST_PHASE_WAIT_STARTUP,
    RS485_TEST_PHASE_PREPARE,
    RS485_TEST_PHASE_WAIT_RELAY,
    RS485_TEST_PHASE_WAIT_RETURN,
    RS485_TEST_PHASE_COMPLETE,
    RS485_TEST_PHASE_FAILED
} rs485_test_phase_t;

typedef enum
{
    RS485_TEST_WAIT_PENDING = 0U,
    RS485_TEST_WAIT_PASSED,
    RS485_TEST_WAIT_FAILED
} rs485_test_wait_result_t;

typedef struct
{
    bsp_rs485_port_t initiator;
    uint32_t size;
    uint8_t seed;
} rs485_test_case_t;

typedef struct
{
    uint32_t baud_rate;
    uint32_t result;
    uint32_t phase;
    uint32_t passed_mask;
    uint32_t failure_code;
    uint32_t test_index;
    uint32_t last_rx_size;
    uint32_t port1_error_count;
    uint32_t port1_dropped_count;
    uint32_t port1_half_event_count;
    uint32_t port1_full_event_count;
    uint32_t port1_timeout_event_count;
    uint32_t port2_error_count;
    uint32_t port2_dropped_count;
    uint32_t port2_half_event_count;
    uint32_t port2_full_event_count;
    uint32_t port2_timeout_event_count;
} rs485_test_status_t;

typedef struct
{
    rs485_test_phase_t phase;
    uint32_t phase_start_time_ms;
    uint32_t test_index;
} rs485_test_context_t;

/* 两路轮流发送，每路都试短、中、长三种数据，检查接收缓存没填满、填到一半和填满时是否正常。 */
static const rs485_test_case_t test_cases[RS485_TEST_CASE_COUNT] =
{
    {BSP_RS485_PORT_1, BOARD_RS485_ECHO_SHORT_SIZE, 0x11U},
    {BSP_RS485_PORT_2, BOARD_RS485_ECHO_SHORT_SIZE, 0x22U},
    {BSP_RS485_PORT_1, BOARD_RS485_DMA_BUFFER_SIZE / 2U, 0x33U},
    {BSP_RS485_PORT_2, BOARD_RS485_DMA_BUFFER_SIZE / 2U, 0x44U},
    {BSP_RS485_PORT_1, BOARD_RS485_DMA_BUFFER_SIZE, 0x55U},
    {BSP_RS485_PORT_2, BOARD_RS485_DMA_BUFFER_SIZE, 0x66U}
};

static uint8_t test_payload[BOARD_RS485_DMA_BUFFER_SIZE];
static uint8_t relay_payload[BOARD_RS485_DMA_BUFFER_SIZE];
static uint8_t return_payload[BOARD_RS485_DMA_BUFFER_SIZE];
static volatile rs485_test_status_t rs485_test_status;
static rs485_test_context_t rs485_test_context;

static bsp_rs485_port_t rs485_test_other_port(bsp_rs485_port_t port)
{
    return (port == BSP_RS485_PORT_1) ? BSP_RS485_PORT_2 : BSP_RS485_PORT_1;
}

static void rs485_test_prepare_payload(const rs485_test_case_t *test_case)
{
    uint32_t index;

    for (index = 0U; index < test_case->size; index++)
    {
        test_payload[index] = (uint8_t)(test_case->seed + (index * 3U));
        relay_payload[index] = 0U;
        return_payload[index] = 0U;
    }
}

static bool rs485_test_payload_matches(const uint8_t *payload,
                                       const rs485_test_case_t *test_case)
{
    uint32_t index;

    for (index = 0U; index < test_case->size; index++)
    {
        if (payload[index] != test_payload[index])
        {
            return false;
        }
    }
    return true;
}

static bool rs485_test_capture_status(bsp_rs485_status_t *port1_status,
                                      bsp_rs485_status_t *port2_status)
{
    if (!bsp_rs485_get_status(BSP_RS485_PORT_1, port1_status) ||
        !bsp_rs485_get_status(BSP_RS485_PORT_2, port2_status))
    {
        return false;
    }

    rs485_test_status.port1_error_count = port1_status->error_count;
    rs485_test_status.port1_dropped_count = port1_status->rx_dropped_count;
    rs485_test_status.port1_half_event_count = port1_status->rx_half_event_count;
    rs485_test_status.port1_full_event_count = port1_status->rx_full_event_count;
    rs485_test_status.port1_timeout_event_count = port1_status->rx_timeout_event_count;
    rs485_test_status.port2_error_count = port2_status->error_count;
    rs485_test_status.port2_dropped_count = port2_status->rx_dropped_count;
    rs485_test_status.port2_half_event_count = port2_status->rx_half_event_count;
    rs485_test_status.port2_full_event_count = port2_status->rx_full_event_count;
    rs485_test_status.port2_timeout_event_count = port2_status->rx_timeout_event_count;
    return true;
}

static bool rs485_test_status_is_clean(const bsp_rs485_status_t *port1_status,
                                       const bsp_rs485_status_t *port2_status)
{
    return (port1_status->error_count == 0U) &&
           (port1_status->rx_dropped_count == 0U) &&
           (port2_status->error_count == 0U) &&
           (port2_status->rx_dropped_count == 0U);
}

static void rs485_test_set_phase(rs485_test_phase_t phase, uint32_t current_time_ms)
{
    rs485_test_context.phase = phase;
    rs485_test_context.phase_start_time_ms = current_time_ms;
    rs485_test_status.phase = phase;
}

static rs485_test_wait_result_t rs485_test_wait_for_data(bsp_rs485_port_t transmitter,
                                                         bsp_rs485_port_t receiver,
                                                         uint8_t *receive_data,
                                                         uint32_t current_time_ms)
{
    const rs485_test_case_t *test_case = &test_cases[rs485_test_context.test_index];
    bsp_rs485_status_t transmitter_status;
    bsp_rs485_status_t receiver_status;
    bsp_rs485_status_t port1_status;
    bsp_rs485_status_t port2_status;
    uint32_t received_size;

    if (!bsp_rs485_get_status(transmitter, &transmitter_status) ||
        !bsp_rs485_get_status(receiver, &receiver_status) ||
        !rs485_test_capture_status(&port1_status, &port2_status) ||
        !rs485_test_status_is_clean(&port1_status, &port2_status))
    {
        return RS485_TEST_WAIT_FAILED;
    }

    if (!transmitter_status.tx_busy && (receiver_status.rx_pending >= test_case->size))
    {
        if (receiver_status.rx_pending != test_case->size)
        {
            return RS485_TEST_WAIT_FAILED;
        }
        received_size = bsp_rs485_read(receiver, receive_data, test_case->size);
        rs485_test_status.last_rx_size = received_size;
        return ((received_size == test_case->size) &&
                rs485_test_payload_matches(receive_data, test_case)) ?
               RS485_TEST_WAIT_PASSED : RS485_TEST_WAIT_FAILED;
    }

    if ((current_time_ms - rs485_test_context.phase_start_time_ms) >=
        BOARD_RS485_ECHO_TIMEOUT_MS)
    {
        return RS485_TEST_WAIT_FAILED;
    }
    return RS485_TEST_WAIT_PENDING;
}

static void rs485_test_fail(void)
{
    bsp_rs485_status_t port1_status;
    bsp_rs485_status_t port2_status;

    (void)rs485_test_capture_status(&port1_status, &port2_status);
    (void)bsp_rs485_abort(BSP_RS485_PORT_1);
    (void)bsp_rs485_abort(BSP_RS485_PORT_2);
    rs485_test_status.failure_code =
        ((rs485_test_context.test_index + 1U) << 8U) | (uint32_t)rs485_test_context.phase;
    rs485_test_context.phase = RS485_TEST_PHASE_FAILED;
    rs485_test_status.phase = RS485_TEST_PHASE_FAILED;
    rs485_test_status.result = RS485_TEST_RESULT_FAILED;
}

void rs485_echo_test_init(uint32_t current_time_ms)
{
    rs485_test_status.baud_rate = BOARD_RS485_BAUD_RATE;
    rs485_test_status.result = RS485_TEST_RESULT_RUNNING;
    rs485_test_status.phase = RS485_TEST_PHASE_WAIT_STARTUP;
    rs485_test_status.passed_mask = 0U;
    rs485_test_status.failure_code = 0U;
    rs485_test_status.test_index = 0U;
    rs485_test_status.last_rx_size = 0U;
    rs485_test_status.port1_error_count = 0U;
    rs485_test_status.port1_dropped_count = 0U;
    rs485_test_status.port1_half_event_count = 0U;
    rs485_test_status.port1_full_event_count = 0U;
    rs485_test_status.port1_timeout_event_count = 0U;
    rs485_test_status.port2_error_count = 0U;
    rs485_test_status.port2_dropped_count = 0U;
    rs485_test_status.port2_half_event_count = 0U;
    rs485_test_status.port2_full_event_count = 0U;
    rs485_test_status.port2_timeout_event_count = 0U;

    if (!bsp_rs485_configure(BSP_RS485_PORT_1, BOARD_RS485_BAUD_RATE,
                             BOARD_RS485_RX_TIMEOUT_BITS) ||
        !bsp_rs485_configure(BSP_RS485_PORT_2, BOARD_RS485_BAUD_RATE,
                             BOARD_RS485_RX_TIMEOUT_BITS))
    {
        rs485_test_fail();
        return;
    }

    rs485_test_context.phase = RS485_TEST_PHASE_WAIT_STARTUP;
    rs485_test_context.phase_start_time_ms = current_time_ms;
    rs485_test_context.test_index = 0U;
}

void rs485_echo_test_process(uint32_t current_time_ms)
{
    const rs485_test_case_t *test_case;
    bsp_rs485_port_t responder;
    rs485_test_wait_result_t wait_result;

    switch (rs485_test_context.phase)
    {
        case RS485_TEST_PHASE_WAIT_STARTUP:
            if ((current_time_ms - rs485_test_context.phase_start_time_ms) >=
                BOARD_RS485_TRANSCEIVER_STARTUP_MS)
            {
                rs485_test_set_phase(RS485_TEST_PHASE_PREPARE, current_time_ms);
            }
            break;

        case RS485_TEST_PHASE_PREPARE:
            test_case = &test_cases[rs485_test_context.test_index];
            responder = rs485_test_other_port(test_case->initiator);
            if ((bsp_rs485_rx_pending(test_case->initiator) != 0U) ||
                (bsp_rs485_rx_pending(responder) != 0U))
            {
                rs485_test_fail();
                break;
            }

            rs485_test_prepare_payload(test_case);
            if (!bsp_rs485_transmit(test_case->initiator, test_payload, test_case->size))
            {
                rs485_test_fail();
            }
            else
            {
                rs485_test_set_phase(RS485_TEST_PHASE_WAIT_RELAY, current_time_ms);
            }
            break;

        /* 另一端先检查收到的内容，确认无误才原样发回来；最初发送的一端还会再检查一遍。 */
        case RS485_TEST_PHASE_WAIT_RELAY:
            test_case = &test_cases[rs485_test_context.test_index];
            responder = rs485_test_other_port(test_case->initiator);
            wait_result = rs485_test_wait_for_data(test_case->initiator, responder,
                                                   relay_payload, current_time_ms);
            if (wait_result == RS485_TEST_WAIT_PASSED)
            {
                if (!bsp_rs485_transmit(responder, relay_payload, test_case->size))
                {
                    rs485_test_fail();
                }
                else
                {
                    rs485_test_set_phase(RS485_TEST_PHASE_WAIT_RETURN, current_time_ms);
                }
            }
            else if (wait_result == RS485_TEST_WAIT_FAILED)
            {
                rs485_test_fail();
            }
            break;

        case RS485_TEST_PHASE_WAIT_RETURN:
            test_case = &test_cases[rs485_test_context.test_index];
            responder = rs485_test_other_port(test_case->initiator);
            wait_result = rs485_test_wait_for_data(responder, test_case->initiator,
                                                   return_payload, current_time_ms);
            if (wait_result == RS485_TEST_WAIT_PASSED)
            {
                rs485_test_status.passed_mask |= 1UL << rs485_test_context.test_index;
                rs485_test_context.test_index++;
                rs485_test_status.test_index = rs485_test_context.test_index;
                if (rs485_test_context.test_index == RS485_TEST_CASE_COUNT)
                {
                    rs485_test_set_phase(RS485_TEST_PHASE_COMPLETE, current_time_ms);
                }
                else
                {
                    rs485_test_set_phase(RS485_TEST_PHASE_PREPARE, current_time_ms);
                }
            }
            else if (wait_result == RS485_TEST_WAIT_FAILED)
            {
                rs485_test_fail();
            }
            break;

        case RS485_TEST_PHASE_COMPLETE:
            rs485_test_status.result = RS485_TEST_RESULT_PASSED;
            break;

        case RS485_TEST_PHASE_IDLE:
        case RS485_TEST_PHASE_FAILED:
        default:
            break;
    }
}

bool rs485_echo_test_get_summary(rs485_echo_test_summary_t *summary)
{
    if (summary == NULL)
    {
        return false;
    }

    summary->result = rs485_test_status.result;
    summary->passed_mask = rs485_test_status.passed_mask;
    summary->failure_code = rs485_test_status.failure_code;
    summary->last_rx_size = rs485_test_status.last_rx_size;
    summary->port1_error_count = rs485_test_status.port1_error_count;
    summary->port1_dropped_count = rs485_test_status.port1_dropped_count;
    summary->port2_error_count = rs485_test_status.port2_error_count;
    summary->port2_dropped_count = rs485_test_status.port2_dropped_count;
    return true;
}
