#include "uart_loopback_test.h"

#include <stdbool.h>

#include "board_config.h"
#include "bsp_uart.h"

#define UART_TEST_CASE_COUNT   (6U)

typedef enum
{
    UART_TEST_PHASE_IDLE = 0U,
    UART_TEST_PHASE_PREPARE,
    UART_TEST_PHASE_WAIT_RX,
    UART_TEST_PHASE_COMPLETE,
    UART_TEST_PHASE_FAILED
} uart_test_phase_t;

typedef struct
{
    bsp_uart_port_t port;
    uint32_t size;
    uint8_t seed;
} uart_test_case_t;

typedef struct
{
    uint32_t baud_rate;
    uint32_t result;
    uint32_t phase;
    uint32_t passed_mask;
    uint32_t failure_code;
    uint32_t test_index;
    uint32_t last_rx_size;
    uint32_t usart1_error_count;
    uint32_t usart1_dropped_count;
    uint32_t usart1_half_event_count;
    uint32_t usart1_full_event_count;
    uint32_t usart1_timeout_event_count;
    uint32_t usart3_error_count;
    uint32_t usart3_dropped_count;
    uint32_t usart3_half_event_count;
    uint32_t usart3_full_event_count;
    uint32_t usart3_timeout_event_count;
} uart_test_status_t;

typedef struct
{
    uart_test_phase_t phase;
    uint32_t phase_start_time_ms;
    uint32_t test_index;
} uart_test_context_t;

/* 两个串口各测自己的 TX-RX 跳线，分别用短、中、长数据检查能否完整收到。 */
static const uart_test_case_t test_cases[UART_TEST_CASE_COUNT] =
{
    {BSP_UART_PORT_USART1, BOARD_UART_LOOPBACK_SHORT_SIZE, 0x17U},
    {BSP_UART_PORT_USART1, BOARD_UART_DMA_BUFFER_SIZE / 2U, 0x36U},
    {BSP_UART_PORT_USART1, BOARD_UART_DMA_BUFFER_SIZE, 0x58U},
    {BSP_UART_PORT_USART3, BOARD_UART_LOOPBACK_SHORT_SIZE, 0x71U},
    {BSP_UART_PORT_USART3, BOARD_UART_DMA_BUFFER_SIZE / 2U, 0x93U},
    {BSP_UART_PORT_USART3, BOARD_UART_DMA_BUFFER_SIZE, 0xB4U}
};

static uint8_t test_payload[BOARD_UART_DMA_BUFFER_SIZE];
static uint8_t receive_payload[BOARD_UART_DMA_BUFFER_SIZE];
static volatile uart_test_status_t uart_test_status;
static uart_test_context_t uart_test_context;

static void uart_test_prepare_payload(const uart_test_case_t *test_case)
{
    uint32_t index;

    for (index = 0U; index < test_case->size; index++)
    {
        test_payload[index] = (uint8_t)(test_case->seed + (index * 3U));
        receive_payload[index] = 0U;
    }
}

static bool uart_test_payload_matches(const uart_test_case_t *test_case)
{
    uint32_t index;

    for (index = 0U; index < test_case->size; index++)
    {
        if (receive_payload[index] != test_payload[index])
        {
            return false;
        }
    }
    return true;
}

static bool uart_test_capture_status(bsp_uart_status_t *usart1_status,
                                     bsp_uart_status_t *usart3_status)
{
    if (!bsp_uart_get_status(BSP_UART_PORT_USART1, usart1_status) ||
        !bsp_uart_get_status(BSP_UART_PORT_USART3, usart3_status))
    {
        return false;
    }

    uart_test_status.usart1_error_count = usart1_status->error_count;
    uart_test_status.usart1_dropped_count = usart1_status->rx_dropped_count;
    uart_test_status.usart1_half_event_count = usart1_status->rx_half_event_count;
    uart_test_status.usart1_full_event_count = usart1_status->rx_full_event_count;
    uart_test_status.usart1_timeout_event_count = usart1_status->rx_timeout_event_count;
    uart_test_status.usart3_error_count = usart3_status->error_count;
    uart_test_status.usart3_dropped_count = usart3_status->rx_dropped_count;
    uart_test_status.usart3_half_event_count = usart3_status->rx_half_event_count;
    uart_test_status.usart3_full_event_count = usart3_status->rx_full_event_count;
    uart_test_status.usart3_timeout_event_count = usart3_status->rx_timeout_event_count;
    return true;
}

static bool uart_test_status_is_clean(const bsp_uart_status_t *usart1_status,
                                      const bsp_uart_status_t *usart3_status)
{
    return (usart1_status->error_count == 0U) &&
           (usart1_status->rx_dropped_count == 0U) &&
           (usart3_status->error_count == 0U) &&
           (usart3_status->rx_dropped_count == 0U);
}

static void uart_test_set_phase(uart_test_phase_t phase, uint32_t current_time_ms)
{
    uart_test_context.phase = phase;
    uart_test_context.phase_start_time_ms = current_time_ms;
    uart_test_status.phase = phase;
}

static void uart_test_fail(void)
{
    bsp_uart_status_t usart1_status;
    bsp_uart_status_t usart3_status;

    (void)uart_test_capture_status(&usart1_status, &usart3_status);
    (void)bsp_uart_abort(BSP_UART_PORT_USART1);
    (void)bsp_uart_abort(BSP_UART_PORT_USART3);
    uart_test_status.failure_code =
        ((uart_test_context.test_index + 1U) << 8U) | (uint32_t)uart_test_context.phase;
    uart_test_context.phase = UART_TEST_PHASE_FAILED;
    uart_test_status.phase = UART_TEST_PHASE_FAILED;
    uart_test_status.result = UART_TEST_RESULT_FAILED;
}

void uart_loopback_test_init(uint32_t current_time_ms)
{
    uart_test_status.baud_rate = BOARD_UART_LOOPBACK_BAUD_RATE;
    uart_test_status.result = UART_TEST_RESULT_RUNNING;
    uart_test_status.phase = UART_TEST_PHASE_PREPARE;
    uart_test_status.passed_mask = 0U;
    uart_test_status.failure_code = 0U;
    uart_test_status.test_index = 0U;
    uart_test_status.last_rx_size = 0U;
    uart_test_status.usart1_error_count = 0U;
    uart_test_status.usart1_dropped_count = 0U;
    uart_test_status.usart1_half_event_count = 0U;
    uart_test_status.usart1_full_event_count = 0U;
    uart_test_status.usart1_timeout_event_count = 0U;
    uart_test_status.usart3_error_count = 0U;
    uart_test_status.usart3_dropped_count = 0U;
    uart_test_status.usart3_half_event_count = 0U;
    uart_test_status.usart3_full_event_count = 0U;
    uart_test_status.usart3_timeout_event_count = 0U;

    uart_test_context.phase = UART_TEST_PHASE_PREPARE;
    uart_test_context.phase_start_time_ms = current_time_ms;
    uart_test_context.test_index = 0U;

    if (!bsp_uart_configure(BSP_UART_PORT_USART1, BOARD_UART_LOOPBACK_BAUD_RATE,
                            BOARD_UART_RX_TIMEOUT_BITS) ||
        !bsp_uart_configure(BSP_UART_PORT_USART3, BOARD_UART_LOOPBACK_BAUD_RATE,
                            BOARD_UART_RX_TIMEOUT_BITS))
    {
        uart_test_fail();
    }
}

void uart_loopback_test_process(uint32_t current_time_ms)
{
    const uart_test_case_t *test_case;
    bsp_uart_status_t active_status;
    bsp_uart_status_t usart1_status;
    bsp_uart_status_t usart3_status;
    uint32_t received_size;

    switch (uart_test_context.phase)
    {
        case UART_TEST_PHASE_PREPARE:
            if ((bsp_uart_rx_pending(BSP_UART_PORT_USART1) != 0U) ||
                (bsp_uart_rx_pending(BSP_UART_PORT_USART3) != 0U))
            {
                uart_test_fail();
                break;
            }

            test_case = &test_cases[uart_test_context.test_index];
            uart_test_prepare_payload(test_case);
            if (!bsp_uart_transmit(test_case->port, test_payload, test_case->size))
            {
                uart_test_fail();
            }
            else
            {
                uart_test_set_phase(UART_TEST_PHASE_WAIT_RX, current_time_ms);
            }
            break;

        case UART_TEST_PHASE_WAIT_RX:
            test_case = &test_cases[uart_test_context.test_index];
            if (!bsp_uart_get_status(test_case->port, &active_status) ||
                !uart_test_capture_status(&usart1_status, &usart3_status) ||
                !uart_test_status_is_clean(&usart1_status, &usart3_status))
            {
                uart_test_fail();
                break;
            }

            if (!active_status.tx_busy && (active_status.rx_pending >= test_case->size))
            {
                if (active_status.rx_pending != test_case->size)
                {
                    uart_test_fail();
                    break;
                }

                received_size = bsp_uart_read(test_case->port, receive_payload,
                                              test_case->size);
                uart_test_status.last_rx_size = received_size;
                if ((received_size != test_case->size) ||
                    !uart_test_payload_matches(test_case))
                {
                    uart_test_fail();
                    break;
                }

                uart_test_status.passed_mask |= 1UL << uart_test_context.test_index;
                uart_test_context.test_index++;
                uart_test_status.test_index = uart_test_context.test_index;
                if (uart_test_context.test_index == UART_TEST_CASE_COUNT)
                {
                    uart_test_set_phase(UART_TEST_PHASE_COMPLETE, current_time_ms);
                }
                else
                {
                    uart_test_set_phase(UART_TEST_PHASE_PREPARE, current_time_ms);
                }
            }
            else if ((current_time_ms - uart_test_context.phase_start_time_ms) >=
                     BOARD_UART_LOOPBACK_TIMEOUT_MS)
            {
                uart_test_fail();
            }
            break;

        case UART_TEST_PHASE_COMPLETE:
            if (!uart_test_capture_status(&usart1_status, &usart3_status) ||
                !uart_test_status_is_clean(&usart1_status, &usart3_status))
            {
                uart_test_fail();
            }
            else
            {
                uart_test_status.result = UART_TEST_RESULT_PASSED;
            }
            break;

        case UART_TEST_PHASE_IDLE:
        case UART_TEST_PHASE_FAILED:
        default:
            break;
    }
}

bool uart_loopback_test_get_summary(uart_loopback_test_summary_t *summary)
{
    if (summary == NULL)
    {
        return false;
    }

    summary->result = uart_test_status.result;
    summary->passed_mask = uart_test_status.passed_mask;
    summary->failure_code = uart_test_status.failure_code;
    summary->last_rx_size = uart_test_status.last_rx_size;
    summary->usart1_error_count = uart_test_status.usart1_error_count;
    summary->usart1_dropped_count = uart_test_status.usart1_dropped_count;
    summary->usart3_error_count = uart_test_status.usart3_error_count;
    summary->usart3_dropped_count = uart_test_status.usart3_dropped_count;
    return true;
}
