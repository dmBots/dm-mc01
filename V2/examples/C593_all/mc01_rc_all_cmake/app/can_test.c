#include "can_test.h"

#include "board_config.h"
#include "bsp_canfd.h"

#define CAN_TEST_ID_CAN1_INTERNAL    (0x101U)
#define CAN_TEST_ID_CAN2_INTERNAL    (0x102U)
#define CAN_TEST_ID_CAN1_TO_CAN2     (0x201U)
#define CAN_TEST_ID_CAN2_TO_CAN1     (0x202U)

#define CAN_TEST_SEED_CAN1_INTERNAL  (0x11U)
#define CAN_TEST_SEED_CAN2_INTERNAL  (0x22U)
#define CAN_TEST_SEED_CAN1_TO_CAN2   (0x33U)
#define CAN_TEST_SEED_CAN2_TO_CAN1   (0x44U)

#define CAN_TEST_CAN1_INTERNAL_PASSED   (1UL << 0)
#define CAN_TEST_CAN2_INTERNAL_PASSED   (1UL << 1)
#define CAN_TEST_CAN1_TO_CAN2_PASSED    (1UL << 2)
#define CAN_TEST_CAN2_TO_CAN1_PASSED    (1UL << 3)

#if BOARD_CAN_PRESET == BOARD_CAN_PRESET_CLASSIC_1M
#define CAN_TEST_SELECTED_PRESET       BSP_CANFD_PRESET_CLASSIC_1M
#define CAN_TEST_PAYLOAD_LENGTH        (8U)
#else
#if BOARD_CAN_PRESET == BOARD_CAN_PRESET_CANFD_5M
#define CAN_TEST_SELECTED_PRESET       BSP_CANFD_PRESET_FD_5M
#else
#define CAN_TEST_SELECTED_PRESET       BSP_CANFD_PRESET_FD_8M
#endif
#define CAN_TEST_PAYLOAD_LENGTH        (64U)
#endif

/* 先在芯片内部让数据发出去又回到自己，再切换到实际接线，检查 CAN1 发到 CAN2 和反向发送。 */
typedef enum
{
    CAN_TEST_PHASE_IDLE = 0U,
    CAN_TEST_PHASE_WAIT_POWER,
    CAN_TEST_PHASE_CONFIG_INTERNAL,
    CAN_TEST_PHASE_SEND_CAN1_INTERNAL,
    CAN_TEST_PHASE_WAIT_CAN1_INTERNAL,
    CAN_TEST_PHASE_SEND_CAN2_INTERNAL,
    CAN_TEST_PHASE_WAIT_CAN2_INTERNAL,
    CAN_TEST_PHASE_CONFIG_NORMAL,
    CAN_TEST_PHASE_SEND_CAN1_TO_CAN2,
    CAN_TEST_PHASE_WAIT_CAN1_TO_CAN2,
    CAN_TEST_PHASE_SEND_CAN2_TO_CAN1,
    CAN_TEST_PHASE_WAIT_CAN2_TO_CAN1,
    CAN_TEST_PHASE_COMPLETE,
    CAN_TEST_PHASE_FAILED
} can_test_phase_t;

typedef enum
{
    CAN_RX_PENDING = 0U,
    CAN_RX_PASSED,
    CAN_RX_FAILED
} can_rx_result_t;

typedef struct
{
    uint32_t preset;
    uint32_t result;
    uint32_t phase;
    uint32_t passed_mask;
    uint32_t failure_code;
    uint32_t last_rx_identifier;
    uint32_t can1_tx_error_count;
    uint32_t can1_rx_error_count;
    uint32_t can1_rx_dropped_count;
    uint32_t can2_tx_error_count;
    uint32_t can2_rx_error_count;
    uint32_t can2_rx_dropped_count;
} can_test_status_t;

typedef struct
{
    can_test_phase_t phase;
    uint32_t phase_start_time_ms;
    uint32_t expected_identifier;
    uint8_t expected_seed;
} can_test_context_t;

static volatile can_test_status_t test_status;
static can_test_context_t test_context;

static void can_test_capture_error_counters(void)
{
    bsp_canfd_error_counters_t counters;

    if (bsp_canfd_get_error_counters(BSP_CANFD_PORT_1, &counters))
    {
        test_status.can1_tx_error_count = counters.tx_error_count;
        test_status.can1_rx_error_count = counters.rx_error_count;
        test_status.can1_rx_dropped_count = counters.rx_dropped_count;
    }
    if (bsp_canfd_get_error_counters(BSP_CANFD_PORT_2, &counters))
    {
        test_status.can2_tx_error_count = counters.tx_error_count;
        test_status.can2_rx_error_count = counters.rx_error_count;
        test_status.can2_rx_dropped_count = counters.rx_dropped_count;
    }
}

static void can_test_fail(uint32_t failure_code)
{
    can_test_capture_error_counters();
    (void)bsp_canfd_stop(BSP_CANFD_PORT_1);
    (void)bsp_canfd_stop(BSP_CANFD_PORT_2);
    test_context.phase = CAN_TEST_PHASE_FAILED;
    test_status.phase = CAN_TEST_PHASE_FAILED;
    test_status.failure_code = failure_code;
    test_status.result = CAN_TEST_RESULT_FAILED;
}

static bool can_test_configure_pair(bsp_canfd_mode_t mode)
{
    if (!bsp_canfd_configure(BSP_CANFD_PORT_1, CAN_TEST_SELECTED_PRESET, mode) ||
        !bsp_canfd_configure(BSP_CANFD_PORT_2, CAN_TEST_SELECTED_PRESET, mode) ||
        !bsp_canfd_start(BSP_CANFD_PORT_1) ||
        !bsp_canfd_start(BSP_CANFD_PORT_2))
    {
        return false;
    }
    return true;
}

static bool can_test_send(bsp_canfd_port_t port, uint32_t identifier, uint8_t seed)
{
    bsp_canfd_message_t message;
    uint32_t index;

    message.identifier = identifier;
    message.length = CAN_TEST_PAYLOAD_LENGTH;
    message.fd_frame = CAN_TEST_SELECTED_PRESET != BSP_CANFD_PRESET_CLASSIC_1M;
    message.bit_rate_switch = message.fd_frame;
    for (index = 0U; index < message.length; index++)
    {
        message.data[index] = (uint8_t)(seed + index);
    }

    test_context.expected_identifier = identifier;
    test_context.expected_seed = seed;
    return bsp_canfd_transmit(port, &message);
}

static can_rx_result_t can_test_check_receive(bsp_canfd_port_t port)
{
    bsp_canfd_message_t message;
    uint32_t index;
    bool expect_fd = CAN_TEST_SELECTED_PRESET != BSP_CANFD_PRESET_CLASSIC_1M;

    if (bsp_canfd_rx_pending(port) == 0U)
    {
        return CAN_RX_PENDING;
    }
    if (!bsp_canfd_receive(port, &message))
    {
        return CAN_RX_FAILED;
    }

    test_status.last_rx_identifier = message.identifier;
    if ((message.identifier != test_context.expected_identifier) ||
        (message.length != CAN_TEST_PAYLOAD_LENGTH) ||
        (message.fd_frame != expect_fd) ||
        (message.bit_rate_switch != expect_fd))
    {
        return CAN_RX_FAILED;
    }

    for (index = 0U; index < message.length; index++)
    {
        if (message.data[index] != (uint8_t)(test_context.expected_seed + index))
        {
            return CAN_RX_FAILED;
        }
    }
    return CAN_RX_PASSED;
}

static bool can_test_timed_out(uint32_t current_time_ms)
{
    return (current_time_ms - test_context.phase_start_time_ms) >= BOARD_CAN_SELF_TEST_TIMEOUT_MS;
}

static void can_test_begin_wait(can_test_phase_t phase, uint32_t current_time_ms)
{
    test_context.phase = phase;
    test_context.phase_start_time_ms = current_time_ms;
    test_status.phase = phase;
}

void can_test_init(uint32_t current_time_ms)
{
    test_status.preset = BOARD_CAN_PRESET;
    test_status.result = CAN_TEST_RESULT_RUNNING;
    test_status.phase = CAN_TEST_PHASE_WAIT_POWER;
    test_status.passed_mask = 0U;
    test_status.failure_code = 0U;
    test_status.last_rx_identifier = 0U;
    test_status.can1_tx_error_count = 0U;
    test_status.can1_rx_error_count = 0U;
    test_status.can1_rx_dropped_count = 0U;
    test_status.can2_tx_error_count = 0U;
    test_status.can2_rx_error_count = 0U;
    test_status.can2_rx_dropped_count = 0U;

    test_context.phase = CAN_TEST_PHASE_WAIT_POWER;
    test_context.phase_start_time_ms = current_time_ms;
}

void can_test_process(uint32_t current_time_ms)
{
    can_rx_result_t rx_result;

    switch (test_context.phase)
    {
        case CAN_TEST_PHASE_WAIT_POWER:
            if ((current_time_ms - test_context.phase_start_time_ms) >= BOARD_CAN_TRANSCEIVER_STARTUP_MS)
            {
                test_context.phase = CAN_TEST_PHASE_CONFIG_INTERNAL;
                test_status.phase = CAN_TEST_PHASE_CONFIG_INTERNAL;
            }
            break;

        case CAN_TEST_PHASE_CONFIG_INTERNAL:
            if (!can_test_configure_pair(BSP_CANFD_MODE_INTERNAL_LOOPBACK))
            {
                can_test_fail(2U);
            }
            else
            {
                test_context.phase = CAN_TEST_PHASE_SEND_CAN1_INTERNAL;
            }
            break;

        case CAN_TEST_PHASE_SEND_CAN1_INTERNAL:
            if (!can_test_send(BSP_CANFD_PORT_1, CAN_TEST_ID_CAN1_INTERNAL, CAN_TEST_SEED_CAN1_INTERNAL))
            {
                can_test_fail(3U);
            }
            else
            {
                can_test_begin_wait(CAN_TEST_PHASE_WAIT_CAN1_INTERNAL, current_time_ms);
            }
            break;

        case CAN_TEST_PHASE_WAIT_CAN1_INTERNAL:
            rx_result = can_test_check_receive(BSP_CANFD_PORT_1);
            if (rx_result == CAN_RX_PASSED)
            {
                test_status.passed_mask |= CAN_TEST_CAN1_INTERNAL_PASSED;
                test_context.phase = CAN_TEST_PHASE_SEND_CAN2_INTERNAL;
            }
            else if ((rx_result == CAN_RX_FAILED) || can_test_timed_out(current_time_ms))
            {
                can_test_fail(4U);
            }
            break;

        case CAN_TEST_PHASE_SEND_CAN2_INTERNAL:
            if (!can_test_send(BSP_CANFD_PORT_2, CAN_TEST_ID_CAN2_INTERNAL, CAN_TEST_SEED_CAN2_INTERNAL))
            {
                can_test_fail(5U);
            }
            else
            {
                can_test_begin_wait(CAN_TEST_PHASE_WAIT_CAN2_INTERNAL, current_time_ms);
            }
            break;

        case CAN_TEST_PHASE_WAIT_CAN2_INTERNAL:
            rx_result = can_test_check_receive(BSP_CANFD_PORT_2);
            if (rx_result == CAN_RX_PASSED)
            {
                test_status.passed_mask |= CAN_TEST_CAN2_INTERNAL_PASSED;
                test_context.phase = CAN_TEST_PHASE_CONFIG_NORMAL;
            }
            else if ((rx_result == CAN_RX_FAILED) || can_test_timed_out(current_time_ms))
            {
                can_test_fail(6U);
            }
            break;

        case CAN_TEST_PHASE_CONFIG_NORMAL:
            if (!bsp_canfd_stop(BSP_CANFD_PORT_1) ||
                !bsp_canfd_stop(BSP_CANFD_PORT_2) ||
                !can_test_configure_pair(BSP_CANFD_MODE_NORMAL))
            {
                can_test_fail(7U);
            }
            else
            {
                test_context.phase = CAN_TEST_PHASE_SEND_CAN1_TO_CAN2;
            }
            break;

        case CAN_TEST_PHASE_SEND_CAN1_TO_CAN2:
            if (!can_test_send(BSP_CANFD_PORT_1, CAN_TEST_ID_CAN1_TO_CAN2, CAN_TEST_SEED_CAN1_TO_CAN2))
            {
                can_test_fail(8U);
            }
            else
            {
                can_test_begin_wait(CAN_TEST_PHASE_WAIT_CAN1_TO_CAN2, current_time_ms);
            }
            break;

        case CAN_TEST_PHASE_WAIT_CAN1_TO_CAN2:
            rx_result = can_test_check_receive(BSP_CANFD_PORT_2);
            if (rx_result == CAN_RX_PASSED)
            {
                test_status.passed_mask |= CAN_TEST_CAN1_TO_CAN2_PASSED;
                test_context.phase = CAN_TEST_PHASE_SEND_CAN2_TO_CAN1;
            }
            else if ((rx_result == CAN_RX_FAILED) || can_test_timed_out(current_time_ms))
            {
                can_test_fail(9U);
            }
            break;

        case CAN_TEST_PHASE_SEND_CAN2_TO_CAN1:
            if (!can_test_send(BSP_CANFD_PORT_2, CAN_TEST_ID_CAN2_TO_CAN1, CAN_TEST_SEED_CAN2_TO_CAN1))
            {
                can_test_fail(10U);
            }
            else
            {
                can_test_begin_wait(CAN_TEST_PHASE_WAIT_CAN2_TO_CAN1, current_time_ms);
            }
            break;

        case CAN_TEST_PHASE_WAIT_CAN2_TO_CAN1:
            rx_result = can_test_check_receive(BSP_CANFD_PORT_1);
            if (rx_result == CAN_RX_PASSED)
            {
                test_status.passed_mask |= CAN_TEST_CAN2_TO_CAN1_PASSED;
                test_context.phase = CAN_TEST_PHASE_COMPLETE;
            }
            else if ((rx_result == CAN_RX_FAILED) || can_test_timed_out(current_time_ms))
            {
                can_test_fail(11U);
            }
            break;

        case CAN_TEST_PHASE_COMPLETE:
            can_test_capture_error_counters();
            test_status.phase = CAN_TEST_PHASE_COMPLETE;
            test_status.result = CAN_TEST_RESULT_PASSED;
            break;

        case CAN_TEST_PHASE_IDLE:
        case CAN_TEST_PHASE_FAILED:
        default:
            break;
    }
}

bool can_test_get_summary(can_test_summary_t *summary)
{
    if (summary == NULL)
    {
        return false;
    }

    summary->result = test_status.result;
    summary->passed_mask = test_status.passed_mask;
    summary->failure_code = test_status.failure_code;
    summary->last_rx_identifier = test_status.last_rx_identifier;
    summary->can1_tx_error_count = test_status.can1_tx_error_count;
    summary->can1_rx_error_count = test_status.can1_rx_error_count;
    summary->can1_rx_dropped_count = test_status.can1_rx_dropped_count;
    summary->can2_tx_error_count = test_status.can2_tx_error_count;
    summary->can2_rx_error_count = test_status.can2_rx_error_count;
    summary->can2_rx_dropped_count = test_status.can2_rx_dropped_count;
    return true;
}
