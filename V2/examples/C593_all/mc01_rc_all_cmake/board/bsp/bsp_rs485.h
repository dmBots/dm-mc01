/*
 * 控制两路 RS485 接口发送和接收字节，完整接线示例见 app/rs485_echo_test.h。
 * 发送和接收共用总线，DE 信号用于切换方向；本程序由串口硬件自动切换，不需手动控制。
 * DMA 负责搬运数据，发送函数返回后，数据可能还没有全部发完。
 */
#ifndef BSP_RS485_H
#define BSP_RS485_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    /* 板上 P5 接口：UART5，PC12 发送、PD2 接收、PC8 控制收发方向。 */
    BSP_RS485_PORT_1 = 0U,
    /* 板上 P4 接口：USART2，PA2 发送、PA3 接收、PA1 控制收发方向。 */
    BSP_RS485_PORT_2
} bsp_rs485_port_t;

typedef struct
{
    bool tx_busy;
    uint32_t rx_pending;
    uint32_t rx_dropped_count;
    uint32_t error_count;
    uint32_t rx_half_event_count;
    uint32_t rx_full_event_count;
    uint32_t rx_timeout_event_count;
} bsp_rs485_status_t;

/* configure 设置通信速度并开始接收；baud_rate 是波特率。
 * receiver_timeout_bits 表示空闲多少个位的时间后取出短包，不是等待多少毫秒。
 */
bool bsp_rs485_configure(bsp_rs485_port_t port, uint32_t baud_rate,
                         uint32_t receiver_timeout_bits);
/* data 是待发送数组；tx_busy=true 表示还在发送，此时不能修改或销毁数组。 */
bool bsp_rs485_transmit(bsp_rs485_port_t port, const uint8_t *data, uint32_t size);
/* 将收到的字节取到 data 数组，最多取 capacity 字节；返回 0 表示本次没取到数据。 */
uint32_t bsp_rs485_read(bsp_rs485_port_t port, uint8_t *data, uint32_t capacity);
uint32_t bsp_rs485_rx_pending(bsp_rs485_port_t port);
bool bsp_rs485_get_status(bsp_rs485_port_t port, bsp_rs485_status_t *status);
/* abort 会停止收发，不是临时暂停；之后需重新 configure 才能继续通信。 */
bool bsp_rs485_abort(bsp_rs485_port_t port);

#endif
