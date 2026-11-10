#include "bsp_uart.h"

#include <stddef.h>

#include "board_config.h"
#include "bsp_uart_internal.h"
#include "mx_usart1.h"
#include "mx_usart3.h"

#define BSP_UART_PORT_COUNT   (2U)

typedef struct
{
    uint8_t dma_buffer[BOARD_UART_DMA_BUFFER_SIZE];
    uint8_t rx_queue[BOARD_UART_RX_QUEUE_SIZE];
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
} bsp_uart_context_t;

static bsp_uart_context_t uart_contexts[BSP_UART_PORT_COUNT];

static hal_uart_handle_t *bsp_uart_get_handle(bsp_uart_port_t port)
{
    if (port == BSP_UART_PORT_USART1)
    {
        return mx_usart1_uart_gethandle();
    }
    if (port == BSP_UART_PORT_USART3)
    {
        return mx_usart3_uart_gethandle();
    }
    return NULL;
}

static bsp_uart_context_t *bsp_uart_get_context(hal_uart_handle_t *handle)
{
    if (handle == mx_usart1_uart_gethandle())
    {
        return &uart_contexts[BSP_UART_PORT_USART1];
    }
    if (handle == mx_usart3_uart_gethandle())
    {
        return &uart_contexts[BSP_UART_PORT_USART3];
    }
    return NULL;
}

static void bsp_uart_queue_byte_from_isr(bsp_uart_context_t *context, uint8_t data)
{
    uint32_t next_index = (context->queue_write_index + 1U) % BOARD_UART_RX_QUEUE_SIZE;

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

/* 缓存到一半、全部填满或接收暂停时，都可能通知这里；记住上次取到的位置，只保存新增字节。 */
static void bsp_uart_commit_dma_from_isr(bsp_uart_context_t *context, uint32_t position)
{
    uint32_t index;

    if (position > BOARD_UART_DMA_BUFFER_SIZE)
    {
        return;
    }

    index = context->dma_position;
    if (position < index)
    {
        while (index < BOARD_UART_DMA_BUFFER_SIZE)
        {
            bsp_uart_queue_byte_from_isr(context, context->dma_buffer[index]);
            index++;
        }
        index = 0U;
    }

    while (index < position)
    {
        bsp_uart_queue_byte_from_isr(context, context->dma_buffer[index]);
        index++;
    }
    context->dma_position = (position == BOARD_UART_DMA_BUFFER_SIZE) ? 0U : position;
}

bool bsp_uart_configure(bsp_uart_port_t port, uint32_t baud_rate,
                        uint32_t receiver_timeout_bits)
{
    hal_uart_handle_t *handle = bsp_uart_get_handle(port);
    bsp_uart_context_t *context;

    if ((handle == NULL) || (baud_rate == 0U) || (receiver_timeout_bits == 0U) ||
        (HAL_UART_GetTxState(handle) != HAL_UART_TX_STATE_IDLE) ||
        (HAL_UART_GetRxState(handle) != HAL_UART_RX_STATE_IDLE) ||
        (HAL_UART_SetBaudRate(handle, baud_rate) != HAL_OK))
    {
        return false;
    }

    context = &uart_contexts[(uint32_t)port];
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
                                    BOARD_UART_DMA_BUFFER_SIZE,
                                    receiver_timeout_bits) != HAL_OK)
    {
        context->configured = false;
        return false;
    }
    return true;
}

bool bsp_uart_transmit(bsp_uart_port_t port, const uint8_t *data, uint32_t size)
{
    hal_uart_handle_t *handle = bsp_uart_get_handle(port);
    bsp_uart_context_t *context;

    if ((handle == NULL) || (data == NULL) || (size == 0U))
    {
        return false;
    }

    context = &uart_contexts[(uint32_t)port];
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

uint32_t bsp_uart_read(bsp_uart_port_t port, uint8_t *data, uint32_t capacity)
{
    bsp_uart_context_t *context;
    uint32_t count = 0U;

    if ((port >= BSP_UART_PORT_COUNT) || (data == NULL))
    {
        return 0U;
    }

    context = &uart_contexts[(uint32_t)port];
    while ((count < capacity) && (context->queue_read_index != context->queue_write_index))
    {
        data[count] = context->rx_queue[context->queue_read_index];
        context->queue_read_index = (context->queue_read_index + 1U) % BOARD_UART_RX_QUEUE_SIZE;
        count++;
    }
    return count;
}

uint32_t bsp_uart_rx_pending(bsp_uart_port_t port)
{
    bsp_uart_context_t *context;

    if (port >= BSP_UART_PORT_COUNT)
    {
        return 0U;
    }

    context = &uart_contexts[(uint32_t)port];
    return (context->queue_write_index + BOARD_UART_RX_QUEUE_SIZE - context->queue_read_index) %
           BOARD_UART_RX_QUEUE_SIZE;
}

bool bsp_uart_get_status(bsp_uart_port_t port, bsp_uart_status_t *status)
{
    bsp_uart_context_t *context;

    if ((port >= BSP_UART_PORT_COUNT) || (status == NULL))
    {
        return false;
    }

    context = &uart_contexts[(uint32_t)port];
    status->tx_busy = context->tx_busy;
    status->rx_pending = bsp_uart_rx_pending(port);
    status->rx_dropped_count = context->rx_dropped_count;
    status->error_count = context->error_count;
    status->rx_half_event_count = context->rx_half_event_count;
    status->rx_full_event_count = context->rx_full_event_count;
    status->rx_timeout_event_count = context->rx_timeout_event_count;
    return context->configured;
}

bool bsp_uart_abort(bsp_uart_port_t port)
{
    hal_uart_handle_t *handle = bsp_uart_get_handle(port);
    bsp_uart_context_t *context;

    if (handle == NULL)
    {
        return false;
    }
    if (HAL_UART_Abort(handle) != HAL_OK)
    {
        return false;
    }

    context = &uart_contexts[(uint32_t)port];
    context->configured = false;
    context->tx_busy = false;
    return true;
}

void bsp_uart_tx_complete_from_isr(hal_uart_handle_t *handle)
{
    bsp_uart_context_t *context = bsp_uart_get_context(handle);

    if (context != NULL)
    {
        context->tx_busy = false;
    }
}

void bsp_uart_rx_half_complete_from_isr(hal_uart_handle_t *handle)
{
    bsp_uart_context_t *context = bsp_uart_get_context(handle);

    if (context != NULL)
    {
        context->rx_half_event_count++;
        bsp_uart_commit_dma_from_isr(context, BOARD_UART_DMA_BUFFER_SIZE / 2U);
    }
}

void bsp_uart_rx_complete_from_isr(hal_uart_handle_t *handle, uint32_t size,
                                   hal_uart_rx_event_types_t rx_event)
{
    bsp_uart_context_t *context = bsp_uart_get_context(handle);

    if (context == NULL)
    {
        return;
    }

    if (rx_event == HAL_UART_RX_EVENT_TC)
    {
        context->rx_full_event_count++;
        bsp_uart_commit_dma_from_isr(context, BOARD_UART_DMA_BUFFER_SIZE);
    }
    else if (rx_event == HAL_UART_RX_EVENT_RTO)
    {
        context->rx_timeout_event_count++;
        bsp_uart_commit_dma_from_isr(context, size);
    }
}

void bsp_uart_error_from_isr(hal_uart_handle_t *handle)
{
    bsp_uart_context_t *context = bsp_uart_get_context(handle);

    if (context != NULL)
    {
        context->tx_busy = false;
        context->configured = false;
        context->error_count++;
    }
}
