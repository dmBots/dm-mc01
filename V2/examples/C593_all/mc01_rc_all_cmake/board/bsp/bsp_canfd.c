#include "bsp_canfd.h"

#include "mx_fdcan1.h"
#include "mx_fdcan2.h"

#define BSP_CANFD_PORT_COUNT          (2U)
#define BSP_CANFD_RX_QUEUE_CAPACITY   (8U)

typedef struct
{
    bsp_canfd_message_t messages[BSP_CANFD_RX_QUEUE_CAPACITY];
    volatile uint32_t write_index;
    volatile uint32_t read_index;
    volatile uint32_t dropped_count;
} bsp_canfd_rx_queue_t;

static bsp_canfd_preset_t configured_preset[BSP_CANFD_PORT_COUNT];
static bool port_configured[BSP_CANFD_PORT_COUNT];
static bsp_canfd_rx_queue_t rx_queues[BSP_CANFD_PORT_COUNT];

static hal_fdcan_handle_t *bsp_canfd_get_handle(bsp_canfd_port_t port)
{
    if (port == BSP_CANFD_PORT_1)
    {
        return mx_fdcan1_gethandle();
    }
    if (port == BSP_CANFD_PORT_2)
    {
        return mx_fdcan2_gethandle();
    }
    return NULL;
}

static bool bsp_canfd_length_to_dlc(uint32_t length, hal_fdcan_data_length_code_t *dlc)
{
    if (length <= 8U)
    {
        *dlc = (hal_fdcan_data_length_code_t)length;
        return true;
    }

    switch (length)
    {
        case 12U:
            *dlc = HAL_FDCAN_DATA_LEN_FDCAN_12_BYTE;
            break;
        case 16U:
            *dlc = HAL_FDCAN_DATA_LEN_FDCAN_16_BYTE;
            break;
        case 20U:
            *dlc = HAL_FDCAN_DATA_LEN_FDCAN_20_BYTE;
            break;
        case 24U:
            *dlc = HAL_FDCAN_DATA_LEN_FDCAN_24_BYTE;
            break;
        case 32U:
            *dlc = HAL_FDCAN_DATA_LEN_FDCAN_32_BYTE;
            break;
        case 48U:
            *dlc = HAL_FDCAN_DATA_LEN_FDCAN_48_BYTE;
            break;
        case 64U:
            *dlc = HAL_FDCAN_DATA_LEN_FDCAN_64_BYTE;
            break;
        default:
            return false;
    }
    return true;
}

static uint32_t bsp_canfd_dlc_to_length(hal_fdcan_data_length_code_t dlc)
{
    static const uint8_t lengths[16] =
    {
        0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 12U, 16U, 20U, 24U, 32U, 48U, 64U
    };

    return lengths[(uint32_t)dlc & 0x0FU];
}

static bool bsp_canfd_read_hardware_fifo(hal_fdcan_handle_t *handle, bsp_canfd_message_t *message)
{
    hal_fdcan_rx_header_t rx_header;

    if (HAL_FDCAN_GetReceivedMessage(handle, HAL_FDCAN_RX_FIFO_0, &rx_header, message->data) != HAL_OK)
    {
        return false;
    }

    message->identifier = rx_header.b.identifier;
    message->length = bsp_canfd_dlc_to_length(rx_header.b.data_length);
    message->fd_frame = rx_header.b.frame_format == HAL_FDCAN_HEADER_FRAME_FORMAT_FD_CAN;
    message->bit_rate_switch = rx_header.b.bit_rate_switch == HAL_FDCAN_BIT_RATE_SWITCH_ON;
    return true;
}

/* 收到消息时，中断会把消息从芯片接收区搬到程序缓存；缓存满了就记一次丢失，要及时 receive 取走。 */
static void bsp_canfd_queue_from_isr(bsp_canfd_port_t port, hal_fdcan_handle_t *handle)
{
    bsp_canfd_rx_queue_t *queue = &rx_queues[(uint32_t)port];
    bsp_canfd_message_t message;
    uint32_t fill_level;
    uint32_t next_index;

    HAL_FDCAN_GetRxFifoFillLevel(handle, HAL_FDCAN_RX_FIFO_0, &fill_level);
    while (fill_level > 0U)
    {
        if (!bsp_canfd_read_hardware_fifo(handle, &message))
        {
            break;
        }

        next_index = (queue->write_index + 1U) % BSP_CANFD_RX_QUEUE_CAPACITY;
        if (next_index == queue->read_index)
        {
            queue->dropped_count++;
        }
        else
        {
            queue->messages[queue->write_index] = message;
            queue->write_index = next_index;
        }

        HAL_FDCAN_GetRxFifoFillLevel(handle, HAL_FDCAN_RX_FIFO_0, &fill_level);
    }
}

bool bsp_canfd_configure(bsp_canfd_port_t port, bsp_canfd_preset_t preset, bsp_canfd_mode_t mode)
{
    hal_fdcan_handle_t *handle = bsp_canfd_get_handle(port);
    hal_fdcan_nominal_bit_timing_t nominal_timing;
    hal_fdcan_data_bit_timing_t data_timing;
    hal_fdcan_tx_delay_comp_config_t tx_delay;
    hal_fdcan_mode_t hal_mode;

    if ((handle == NULL) || (HAL_FDCAN_GetState(handle) != HAL_FDCAN_STATE_IDLE) ||
        (preset > BSP_CANFD_PRESET_FD_8M) || (mode > BSP_CANFD_MODE_INTERNAL_LOOPBACK))
    {
        return false;
    }

    nominal_timing.nominal_prescaler = 1U;
    nominal_timing.nominal_time_seg1 = 139U;
    nominal_timing.nominal_time_seg2 = 20U;
    nominal_timing.nominal_jump_width = 20U;

    if (preset == BSP_CANFD_PRESET_FD_8M)
    {
        data_timing.data_prescaler = 1U;
        data_timing.data_time_seg1 = 15U;
        data_timing.data_time_seg2 = 4U;
        data_timing.data_jump_width = 4U;
        tx_delay.tx_delay_comp_offset = 16U;
    }
    else
    {
        data_timing.data_prescaler = 1U;
        data_timing.data_time_seg1 = 27U;
        data_timing.data_time_seg2 = 4U;
        data_timing.data_jump_width = 4U;
        tx_delay.tx_delay_comp_offset = 28U;
    }
    tx_delay.tx_delay_comp_win_length = 0U;

    hal_mode = (mode == BSP_CANFD_MODE_INTERNAL_LOOPBACK) ?
               HAL_FDCAN_MODE_INTERNAL_LOOPBACK : HAL_FDCAN_MODE_NORMAL;

    if ((HAL_FDCAN_SetNominalBitTiming(handle, &nominal_timing) != HAL_OK) ||
        (HAL_FDCAN_SetDataBitTiming(handle, &data_timing) != HAL_OK) ||
        (HAL_FDCAN_SetMode(handle, hal_mode) != HAL_OK))
    {
        return false;
    }

    if (preset == BSP_CANFD_PRESET_CLASSIC_1M)
    {
        if ((HAL_FDCAN_SetFrameFormat(handle, HAL_FDCAN_FRAME_FORMAT_CLASSIC_CAN) != HAL_OK) ||
            (HAL_FDCAN_DisableTxDelayCompensation(handle) != HAL_OK))
        {
            return false;
        }
    }
    else if ((HAL_FDCAN_SetFrameFormat(handle, HAL_FDCAN_FRAME_FORMAT_FD_BRS) != HAL_OK) ||
             (HAL_FDCAN_SetConfigTxDelayCompensation(handle, &tx_delay) != HAL_OK) ||
             (HAL_FDCAN_EnableTxDelayCompensation(handle) != HAL_OK))
    {
        return false;
    }

    rx_queues[(uint32_t)port].write_index = 0U;
    rx_queues[(uint32_t)port].read_index = 0U;
    rx_queues[(uint32_t)port].dropped_count = 0U;
    configured_preset[(uint32_t)port] = preset;
    port_configured[(uint32_t)port] = true;
    return true;
}

bool bsp_canfd_start(bsp_canfd_port_t port)
{
    hal_fdcan_handle_t *handle = bsp_canfd_get_handle(port);

    return (handle != NULL) && port_configured[(uint32_t)port] &&
           (HAL_FDCAN_Start(handle) == HAL_OK);
}

bool bsp_canfd_stop(bsp_canfd_port_t port)
{
    hal_fdcan_handle_t *handle = bsp_canfd_get_handle(port);

    if (handle == NULL)
    {
        return false;
    }
    if (HAL_FDCAN_GetState(handle) == HAL_FDCAN_STATE_IDLE)
    {
        return true;
    }
    return HAL_FDCAN_Stop(handle) == HAL_OK;
}

bool bsp_canfd_transmit(bsp_canfd_port_t port, const bsp_canfd_message_t *message)
{
    hal_fdcan_handle_t *handle = bsp_canfd_get_handle(port);
    hal_fdcan_tx_header_t tx_header;
    hal_fdcan_data_length_code_t data_length;

    if ((handle == NULL) || (message == NULL) || (message->identifier > 0x7FFU) ||
        !port_configured[(uint32_t)port] ||
        ((configured_preset[(uint32_t)port] == BSP_CANFD_PRESET_CLASSIC_1M) &&
         (message->length > 8U)) ||
        !bsp_canfd_length_to_dlc(message->length, &data_length))
    {
        return false;
    }

    tx_header.d64 = 0U;
    tx_header.b.identifier = message->identifier;
    tx_header.b.identifier_type = HAL_FDCAN_ID_STANDARD;
    tx_header.b.frame_type = HAL_FDCAN_FRAME_DATA;
    tx_header.b.error_state_indicator = HAL_FDCAN_ERROR_STATE_IND_ACTIVE;
    tx_header.b.data_length = data_length;
    tx_header.b.bit_rate_switch = message->bit_rate_switch ?
                                  HAL_FDCAN_BIT_RATE_SWITCH_ON : HAL_FDCAN_BIT_RATE_SWITCH_OFF;
    tx_header.b.frame_format = message->fd_frame ?
                               HAL_FDCAN_HEADER_FRAME_FORMAT_FD_CAN : HAL_FDCAN_HEADER_FRAME_FORMAT_CAN;
    tx_header.b.event_fifo_control = HAL_FDCAN_TX_EVENTS_FIFO_DISCARD;

    return HAL_FDCAN_ReqTransmitMsgFromFIFOQ(handle, &tx_header, message->data) == HAL_OK;
}

bool bsp_canfd_receive(bsp_canfd_port_t port, bsp_canfd_message_t *message)
{
    bsp_canfd_rx_queue_t *queue;

    if ((port >= BSP_CANFD_PORT_COUNT) || (message == NULL))
    {
        return false;
    }

    queue = &rx_queues[(uint32_t)port];
    if (queue->read_index == queue->write_index)
    {
        return false;
    }

    *message = queue->messages[queue->read_index];
    queue->read_index = (queue->read_index + 1U) % BSP_CANFD_RX_QUEUE_CAPACITY;
    return true;
}

uint32_t bsp_canfd_rx_pending(bsp_canfd_port_t port)
{
    bsp_canfd_rx_queue_t *queue;

    if (port >= BSP_CANFD_PORT_COUNT)
    {
        return 0U;
    }

    queue = &rx_queues[(uint32_t)port];
    return (queue->write_index + BSP_CANFD_RX_QUEUE_CAPACITY - queue->read_index) %
           BSP_CANFD_RX_QUEUE_CAPACITY;
}

bool bsp_canfd_get_error_counters(bsp_canfd_port_t port, bsp_canfd_error_counters_t *counters)
{
    hal_fdcan_handle_t *handle = bsp_canfd_get_handle(port);
    hal_fdcan_error_counters_t hal_counters;

    if ((handle == NULL) || (counters == NULL) ||
        (HAL_FDCAN_GetErrorCounters(handle, &hal_counters) != HAL_OK))
    {
        return false;
    }

    counters->tx_error_count = hal_counters.tx_error_cnt;
    counters->rx_error_count = hal_counters.rx_error_cnt;
    counters->rx_dropped_count = rx_queues[(uint32_t)port].dropped_count;
    return true;
}

void HAL_FDCAN_RxFifo0Callback(hal_fdcan_handle_t *handle, uint32_t interrupts)
{
    if ((interrupts & HAL_FDCAN_IT_RX_FIFO_0_NEW_MSG) == 0U)
    {
        return;
    }

    if (handle == mx_fdcan1_gethandle())
    {
        bsp_canfd_queue_from_isr(BSP_CANFD_PORT_1, handle);
    }
    else if (handle == mx_fdcan2_gethandle())
    {
        bsp_canfd_queue_from_isr(BSP_CANFD_PORT_2, handle);
    }
}
