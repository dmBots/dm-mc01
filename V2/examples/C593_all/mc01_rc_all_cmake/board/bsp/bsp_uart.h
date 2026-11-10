/*
 * 通过 USART1/USART3 两个串口收发字节；TX 是发送脚，RX 是接收脚。
 * 收到的数据先保存在程序缓存中，主循环调用 read 把它取出来。
 * DMA 负责自动搬运数据；transmit 返回后不一定已经发送完毕。
 */
#ifndef BSP_UART_H
#define BSP_UART_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    /* 板上 P10 接口：USART1，PA9 是 TX（发送），PA10 是 RX（接收）。 */
    BSP_UART_PORT_USART1 = 0U,
    /* 板上 P13 接口：USART3，PC10 是 TX（发送），PC11 是 RX（接收）。 */
    BSP_UART_PORT_USART3
} bsp_uart_port_t;

typedef struct
{
    bool tx_busy;
    uint32_t rx_pending;
    uint32_t rx_dropped_count;
    uint32_t error_count;
    uint32_t rx_half_event_count;
    uint32_t rx_full_event_count;
    uint32_t rx_timeout_event_count;
} bsp_uart_status_t;

/* configure 设置通信速度并开始接收；baud_rate 是波特率。
 * receiver_timeout_bits 表示空闲多少个位的时间后取出短包，不是等待多少毫秒。
 */
bool bsp_uart_configure(bsp_uart_port_t port, uint32_t baud_rate,
                        uint32_t receiver_timeout_bits);
/* data 是待发送数组；tx_busy=true 表示还在发送，此时不能修改或销毁数组。 */
bool bsp_uart_transmit(bsp_uart_port_t port, const uint8_t *data, uint32_t size);
/* 将收到的字节取到 data 数组，最多取 capacity 字节；返回值是实际取出的字节数。 */
uint32_t bsp_uart_read(bsp_uart_port_t port, uint8_t *data, uint32_t capacity);
uint32_t bsp_uart_rx_pending(bsp_uart_port_t port);
bool bsp_uart_get_status(bsp_uart_port_t port, bsp_uart_status_t *status);
/* abort 会停止收发，不是临时暂停；之后需重新 configure 才能继续通信。 */
bool bsp_uart_abort(bsp_uart_port_t port);

#endif
