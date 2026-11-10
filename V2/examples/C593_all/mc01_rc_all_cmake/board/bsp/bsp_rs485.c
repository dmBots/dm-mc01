#include "bsp_rs485.h"

#include <stddef.h>

#include "board_config.h"
#include "bsp_sbus_internal.h"
#include "bsp_uart_internal.h"
#include "mx_uart5.h"
#include "mx_usart2.h"

#define BSP_RS485_PORT_COUNT   (2U)

typedef struct
{
    uint8_t dma_buffer[BOARD_RS485_DMA_BUFFER_SIZE];
    uint8_t rx_queue[BOARD_RS485_RX_QUEUE_SIZE];
    bool configured;
    volatile bool tx_busy;
    volatile uint32_t dma_position;
    volatile uint32_t queue_write_index;
    volatile uint32_t queue_read_index;
    volatile uint32_t rx_dropped_count;
    volatile uint32_t error_count;
    volatile uint32_t rx_half_event_count;
    volatile uint32_t rx_full_event_count;
    volatile uint32_t rx_timeout_event_count;
} bsp_rs485_context_t;

static bsp_rs485_context_t rs485_contexts[BSP_RS485_PORT_COUNT];

static hal_uart_handle_t *bsp_rs485_get_handle(bsp_rs485_port_t port)
{
    if (port == BSP_RS485_PORT_1)
    {
        return mx_uart5_uart_gethandle();
    }
    if (port == BSP_RS485_PORT_2)
    {
        return mx_usart2_uart_gethandle();
    }
    return NULL;
}

static bsp_rs485_context_t *bsp_rs485_get_context(hal_uart_handle_t *handle)
{
    if (handle == mx_uart5_uart_gethandle())
    {
        return &rs485_contexts[BSP_RS485_PORT_1];
    }
    if (handle == mx_usart2_uart_gethandle())
    {
        return &rs485_contexts[BSP_RS485_PORT_2];
    }
    return NULL;
}

static void bsp_rs485_queue_byte_from_isr(bsp_rs485_context_t *context, uint8_t data)
{
    uint32_t next_index = (context->queue_write_index + 1U) % BOARD_RS485_RX_QUEUE_SIZE;

    if (next_index == context->queue_read_index)
    {
        context->rx_dropped_count++;
    }
    else
    {
        context->rx_queue[context->queue_write_index] = data;
        context->queue_write_index = next_index;
    }
}

/* DMA 会循环填充接收数组；这里只取上次位置之后的新字节，先存好再交给主循环读取。 */
static void bsp_rs485_commit_dma_from_isr(bsp_rs485_context_t *context, uint32_t position)
{
    uint32_t index;

    if (position > BOARD_RS485_DMA_BUFFER_SIZE)
    {
        return;
    }

    index = context->dma_position;
    if (position < index)
    {
        while (index < BOARD_RS485_DMA_BUFFER_SIZE)
        {
            bsp_rs485_queue_byte_from_isr(context, context->dma_buffer[index]);
            index++;
        }
        index = 0U;
    }

    while (index < position)
    {
        bsp_rs485_queue_byte_from_isr(context, context->dma_buffer[index]);
        index++;
    }
    context->dma_position = (position == BOARD_RS485_DMA_BUFFER_SIZE) ? 0U : position;
}

bool bsp_rs485_configure(bsp_rs485_port_t port, uint32_t baud_rate,
                         uint32_t receiver_timeout_bits)
{
    hal_uart_handle_t *handle = bsp_rs485_get_handle(port);
    bsp_rs485_context_t *context;
    hal_uart_rs485_config_t rs485_config;

    if ((handle == NULL) || (baud_rate == 0U) || (receiver_timeout_bits == 0U) ||
        (HAL_UART_GetTxState(handle) != HAL_UART_TX_STATE_IDLE) ||
        (HAL_UART_GetRxState(handle) != HAL_UART_RX_STATE_IDLE))
    {
        return false;
    }

    rs485_config.assertion_time_samples = 0U;
    rs485_config.deassertion_time_samples = 0U;
    rs485_config.polarity = HAL_UART_DE_POLARITY_HIGH;
    if ((HAL_UART_SetBaudRate(handle, baud_rate) != HAL_OK) ||
        (HAL_UART_SetConfigRS485Mode(handle, &rs485_config) != HAL_OK) ||
        (HAL_UART_EnableRS485Mode(handle) != HAL_OK))
    {
        return false;
    }

    context = &rs485_contexts[(uint32_t)port];
    context->configured = true;
    context->tx_busy = false;
    context->dma_position = 0U;
    context->queue_write_index = 0U;
    context->queue_read_index = 0U;
    context->rx_dropped_count = 0U;
    context->error_count = 0U;
    context->rx_half_event_count = 0U;
    context->rx_full_event_count = 0U;
    context->rx_timeout_event_count = 0U;

    if (HAL_UART_ReceiveUntilTMO_DMA(handle, context->dma_buffer,
                                    BOARD_RS485_DMA_BUFFER_SIZE,
                                    receiver_timeout_bits) != HAL_OK)
    {
        context->configured = false;
        return false;
    }
    return true;
}

bool bsp_rs485_transmit(bsp_rs485_port_t port, const uint8_t *data, uint32_t size)
{
    hal_uart_handle_t *handle = bsp_rs485_get_handle(port);
    bsp_rs485_context_t *context;

    if ((handle == NULL) || (data == NULL) || (size == 0U))
    {
        return false;
    }

    context = &rs485_contexts[(uint32_t)port];
    if (!context->configured || context->tx_busy)
    {
        return false;
    }

    context->tx_busy = true;
    if (HAL_UART_Transmit_DMA(handle, data, size) != HAL_OK)
    {
        context->tx_busy = false;
        return false;
    }
    return true;
}

uint32_t bsp_rs485_read(bsp_rs485_port_t port, uint8_t *data, uint32_t capacity)
{
    bsp_rs485_context_t *context;
    uint32_t count = 0U;

    if ((port >= BSP_RS485_PORT_COUNT) || (data == NULL))
    {
        return 0U;
    }

    context = &rs485_contexts[(uint32_t)port];
    while ((count < capacity) && (context->queue_read_index != context->queue_write_index))
    {
        data[count] = context->rx_queue[context->queue_read_index];
        context->queue_read_index = (context->queue_read_index + 1U) % BOARD_RS485_RX_QUEUE_SIZE;
        count++;
    }
    return count;
}

uint32_t bsp_rs485_rx_pending(bsp_rs485_port_t port)
{
    bsp_rs485_context_t *context;

    if (port >= BSP_RS485_PORT_COUNT)
    {
        return 0U;
    }

    context = &rs485_contexts[(uint32_t)port];
    return (context->queue_write_index + BOARD_RS485_RX_QUEUE_SIZE - context->queue_read_index) %
           BOARD_RS485_RX_QUEUE_SIZE;
}

bool bsp_rs485_get_status(bsp_rs485_port_t port, bsp_rs485_status_t *status)
{
    bsp_rs485_context_t *context;

    if ((port >= BSP_RS485_PORT_COUNT) || (status == NULL))
    {
        return false;
    }

    context = &rs485_contexts[(uint32_t)port];
    status->tx_busy = context->tx_busy;
    status->rx_pending = bsp_rs485_rx_pending(port);
    status->rx_dropped_count = context->rx_dropped_count;
    status->error_count = context->error_count;
    status->rx_half_event_count = context->rx_half_event_count;
    status->rx_full_event_count = context->rx_full_event_count;
    status->rx_timeout_event_count = context->rx_timeout_event_count;
    return context->configured;
}

bool bsp_rs485_abort(bsp_rs485_port_t port)
{
    hal_uart_handle_t *handle = bsp_rs485_get_handle(port);
    bsp_rs485_context_t *context;

    if (handle == NULL)
    {
        return false;
    }
    if (HAL_UART_Abort(handle) != HAL_OK)
    {
        return false;
    }

    context = &rs485_contexts[(uint32_t)port];
    context->configured = false;
    context->tx_busy = false;
    return true;
}

/*
 * 串口发送完、收到数据或出错时，HAL 底层会自动调用下面的函数，这叫“回调”。
 * 这里根据是哪个串口，把消息交给 RS485、普通 UART 或 SBUS 模块。
 * 添加串口功能时保留这些转发，不要在另一个文件再定义同名 HAL 回调。
 */
void HAL_UART_TxCpltCallback(hal_uart_handle_t *handle)
{
    bsp_rs485_context_t *context = bsp_rs485_get_context(handle);

    if (context != NULL)
    {
        context->tx_busy = false;
    }
    else
    {
        bsp_uart_tx_complete_from_isr(handle);
    }
}

void HAL_UART_RxHalfCpltCallback(hal_uart_handle_t *handle)
{
    bsp_rs485_context_t *context = bsp_rs485_get_context(handle);

    if (context != NULL)
    {
        context->rx_half_event_count++;
        bsp_rs485_commit_dma_from_isr(context, BOARD_RS485_DMA_BUFFER_SIZE / 2U);
    }
    else
    {
        bsp_uart_rx_half_complete_from_isr(handle);
        bsp_sbus_rx_half_complete_from_isr(handle);
    }
}

void HAL_UART_RxCpltCallback(hal_uart_handle_t *handle, uint32_t size,
                             hal_uart_rx_event_types_t rx_event)
{
    bsp_rs485_context_t *context = bsp_rs485_get_context(handle);

    if (context == NULL)
    {
        bsp_uart_rx_complete_from_isr(handle, size, rx_event);
        bsp_sbus_rx_complete_from_isr(handle, size, rx_event);
        return;
    }

    if (rx_event == HAL_UART_RX_EVENT_TC)
    {
        context->rx_full_event_count++;
        bsp_rs485_commit_dma_from_isr(context, BOARD_RS485_DMA_BUFFER_SIZE);
    }
    else if (rx_event == HAL_UART_RX_EVENT_RTO)
    {
        context->rx_timeout_event_count++;
        bsp_rs485_commit_dma_from_isr(context, size);
    }
}

void HAL_UART_ErrorCallback(hal_uart_handle_t *handle)
{
    bsp_rs485_context_t *context = bsp_rs485_get_context(handle);

    if (context != NULL)
    {
        context->tx_busy = false;
        context->configured = false;
        context->error_count++;
    }
    else
    {
        bsp_uart_error_from_isr(handle);
        bsp_sbus_error_from_isr(handle);
    }
}
