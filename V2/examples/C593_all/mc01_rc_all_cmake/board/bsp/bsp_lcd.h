/*
 * 通过 SPI1 通信线把命令和数据送到 LCD（显示屏），并控制复位和背光。
 * DMA 会在后台读取发送数组；发起发送后不能立刻修改或销毁该数组。
 * 主循环反复调用 process，再用 take_transfer_result 查询是否发送完毕。
 * 如果只想清屏或画图，使用 app/lcd.h 的绘图函数，不必直接编写屏幕命令。
 */
#ifndef BSP_LCD_H
#define BSP_LCD_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    BSP_LCD_TRANSFER_COMMAND = 0U,
    BSP_LCD_TRANSFER_DATA
} bsp_lcd_transfer_type_t;

typedef struct
{
    bool busy;
    uint32_t spi_kernel_clock_hz;
    uint32_t spi_sclk_hz;
    uint32_t dma_transfer_count;
    uint32_t transmitted_byte_count;
    uint32_t error_count;
} bsp_lcd_status_t;

bool bsp_lcd_init(void);
void bsp_lcd_process(void);
/* set_reset(true) 让屏幕保持复位，false 解除复位；set_backlight(true) 打开背光。 */
void bsp_lcd_set_reset(bool asserted);
void bsp_lcd_set_backlight(bool enabled);
/* data 是待发送的数组，size 是字节数；返回 true 仅表示开始发送。
 * 发送结束前不要修改数组，也不要让保存数组的局部变量退出作用域。
 */
bool bsp_lcd_transmit_dma(bsp_lcd_transfer_type_t type,
                          const uint8_t *data,
                          uint32_t size);
/* 返回 true 表示这次发送有结果了，再查看 *success：true 成功、false 失败。
 * 返回 false 表示还没有可取的结果；结果取走后，才能提交下一次发送。
 */
bool bsp_lcd_take_transfer_result(bool *success);
bool bsp_lcd_get_status(bsp_lcd_status_t *status);

#endif
