/*
 * 这是 BSP 内部使用的中断转接声明：收到串口事件时，由 bsp_rs485.c 通知 UART 模块。
 * 正常使用串口只需要包含 bsp_uart.h，不需要在应用中直接调用本文件的函数。
 */
#ifndef BSP_UART_INTERNAL_H
#define BSP_UART_INTERNAL_H

#include "stm32_hal.h"

void bsp_uart_tx_complete_from_isr(hal_uart_handle_t *handle);
void bsp_uart_rx_half_complete_from_isr(hal_uart_handle_t *handle);
void bsp_uart_rx_complete_from_isr(hal_uart_handle_t *handle, uint32_t size,
                                   hal_uart_rx_event_types_t rx_event);
void bsp_uart_error_from_isr(hal_uart_handle_t *handle);

#endif
