/*
 * SPI 是芯片之间交换数据的一种通信方式；本例程用 SPI2 读取 BMI088 惯性传感器。
 * 加速度计和陀螺仪共用通信线，通过两个片选引脚选择本次访问哪一个。
 */
#ifndef BSP_SPI2_H
#define BSP_SPI2_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    BSP_SPI2_CHIP_SELECT_0 = 0U,
    BSP_SPI2_CHIP_SELECT_1
} bsp_spi2_chip_select_t;

bool bsp_spi2_init(void);
/* 发送 tx_data 的同时接收 rx_data，两个数组都必须提供，size 是字节数。
 * 函数内部选择通信对象，等待本次收发结束后才返回；调用方不必再操作片选引脚。
 */
bool bsp_spi2_transfer(bsp_spi2_chip_select_t chip_select,
                       const uint8_t *tx_data,
                       uint8_t *rx_data,
                       uint32_t size);
/* 先调用 init 启用芯片内的计数器，再使用此延时；1000 微秒等于 1 毫秒。
 * 等待期间不会处理主循环的其他任务，只用于传感器要求的短暂等待。
 */
void bsp_spi2_delay_us(uint32_t delay_us);

#endif
