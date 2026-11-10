#include "bsp_sbus.h"

#include <stddef.h>

#include "board_config.h"
#include "bsp_sbus_internal.h"
#include "mx_uart7.h"

typedef struct
{
    uint8_t dma_buffer[BOARD_SBUS_DMA_BUFFER_SIZE];
    uint8_t rx_queue[BOARD_SBUS_RX_QUEUE_SIZE];
    bool configured;
    volatile bool receiving;
    volatile bool restart_pending;
    volatile uint32_t dma_position;
    volatile uint32_t queue_write_index;
    volatile uint32_t queue_read_index;
    volatile uint32_t rx_dropped_count;
    volatile uint32_t error_count;
    volatile uint32_t rx_half_event_count;
    volatile uint32_t rx_full_event_count;
    volatile uint32_t restart_count;
} bsp_sbus_context_t;

static bsp_sbus_context_t sbus_context;

static void bsp_sbus_queue_byte_from_isr(uint8_t data)
{
    uint32_t next_index = (sbus_context.queue_write_index + 1U) % BOARD_SBUS_RX_QUEUE_SIZE;

    if (next_index == sbus_context.queue_read_index)
    {
        sbus_context.rx_dropped_count++;
    }
    else
    {
        sbus_context.rx_queue[sbus_context.queue_write_index] = data;
        sbus_context.queue_write_index = next_index;
    }
}

static void bsp_sbus_commit_dma_from_isr(uint32_t position)
{
    uint32_t index;

    if (position > BOARD_SBUS_DMA_BUFFER_SIZE)
    {
        return;
    }

    index = sbus_context.dma_position;
    if (position < index)
    {
        while (index < BOARD_SBUS_DMA_BUFFER_SIZE)
        {
            bsp_sbus_queue_byte_from_isr(sbus_context.dma_buffer[index]);
            index++;
        }
        index = 0U;
    }

    while (index < position)
    {
        bsp_sbus_queue_byte_from_isr(sbus_context.dma_buffer[index]);
        index++;
    }
    sbus_context.dma_position = (position == BOARD_SBUS_DMA_BUFFER_SIZE) ? 0U : position;
}

static bool bsp_sbus_start_dma(void)
{
    hal_uart_handle_t *handle = mx_uart7_uart_gethandle();

    sbus_context.dma_position = 0U;
    if (HAL_UART_Receive_DMA(handle, sbus_context.dma_buffer,
                            BOARD_SBUS_DMA_BUFFER_SIZE) != HAL_OK)
    {
        return false;
    }

    sbus_context.receiving = true;
    sbus_context.restart_pending = false;
    return true;
}

bool bsp_sbus_start(void)
{
    hal_uart_handle_t *handle = mx_uart7_uart_gethandle();

    if ((handle == NULL) || (HAL_UART_GetRxState(handle) != HAL_UART_RX_STATE_IDLE) ||
        (HAL_UART_DisableReceiverTimeout(handle) != HAL_OK))
    {
        return false;
    }

    sbus_context.configured = true;
    sbus_context.receiving = false;
    sbus_context.restart_pending = false;
    sbus_context.dma_position = 0U;
    sbus_context.queue_write_index = 0U;
    sbus_context.queue_read_index = 0U;
    sbus_context.rx_dropped_count = 0U;
    sbus_context.error_count = 0U;
    sbus_context.rx_half_event_count = 0U;
    sbus_context.rx_full_event_count = 0U;
    sbus_context.restart_count = 0U;

    if (!bsp_sbus_start_dma())
    {
        sbus_context.configured = false;
        return false;
    }
    return true;
}

/* 出错时先记下“需要重新接收”；主循环等串口空闲后再启动，避免在出错通知里反复重启。 */
void bsp_sbus_process(void)
{
    hal_uart_handle_t *handle;

    if (!sbus_context.configured || !sbus_context.restart_pending)
    {
        return;
    }

    handle = mx_uart7_uart_gethandle();
    if ((handle != NULL) && (HAL_UART_GetRxState(handle) == HAL_UART_RX_STATE_IDLE) &&
        bsp_sbus_start_dma())
    {
        sbus_context.restart_count++;
    }
}

uint32_t bsp_sbus_read(uint8_t *data, uint32_t capacity)
{
    uint32_t count = 0U;

    if (data == NULL)
    {
        return 0U;
    }

    while ((count < capacity) &&
           (sbus_context.queue_read_index != sbus_context.queue_write_index))
    {
        data[count] = sbus_context.rx_queue[sbus_context.queue_read_index];
        sbus_context.queue_read_index =
            (sbus_context.queue_read_index + 1U) % BOARD_SBUS_RX_QUEUE_SIZE;
        count++;
    }
    return count;
}

uint32_t bsp_sbus_rx_pending(void)
{
    return (sbus_context.queue_write_index + BOARD_SBUS_RX_QUEUE_SIZE -
            sbus_context.queue_read_index) % BOARD_SBUS_RX_QUEUE_SIZE;
}

bool bsp_sbus_get_status(bsp_sbus_status_t *status)
{
    if (status == NULL)
    {
        return false;
    }

    status->receiving = sbus_context.receiving;
    status->rx_pending = bsp_sbus_rx_pending();
    status->rx_dropped_count = sbus_context.rx_dropped_count;
    status->error_count = sbus_context.error_count;
    status->rx_half_event_count = sbus_context.rx_half_event_count;
    status->rx_full_event_count = sbus_context.rx_full_event_count;
    status->restart_count = sbus_context.restart_count;
    return sbus_context.configured;
}

bool bsp_sbus_stop(void)
{
    hal_uart_handle_t *handle = mx_uart7_uart_gethandle();

    if ((handle == NULL) || (HAL_UART_AbortReceive(handle) != HAL_OK))
    {
        return false;
    }

    sbus_context.configured = false;
    sbus_context.receiving = false;
    sbus_context.restart_pending = false;
    return true;
}

void bsp_sbus_rx_half_complete_from_isr(hal_uart_handle_t *handle)
{
    if (handle == mx_uart7_uart_gethandle())
    {
        sbus_context.rx_half_event_count++;
        bsp_sbus_commit_dma_from_isr(BOARD_SBUS_DMA_BUFFER_SIZE / 2U);
    }
}

void bsp_sbus_rx_complete_from_isr(hal_uart_handle_t *handle, uint32_t size,
                                   hal_uart_rx_event_types_t rx_event)
{
    if ((handle == mx_uart7_uart_gethandle()) && (rx_event == HAL_UART_RX_EVENT_TC))
    {
        sbus_context.rx_full_event_count++;
        bsp_sbus_commit_dma_from_isr(size);
    }
}

void bsp_sbus_error_from_isr(hal_uart_handle_t *handle)
{
    if (handle == mx_uart7_uart_gethandle())
    {
        sbus_context.receiving = false;
        sbus_context.restart_pending = true;
        sbus_context.error_count++;
    }
}
