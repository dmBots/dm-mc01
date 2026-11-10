/*
 * 在 LCD 显示屏上填充色块和显示图片，屏幕大小在 board_config.h 中设置。
 * init 只是开始准备屏幕，不能调用后就立刻画图；主循环要反复调用 process(HAL_GetTick())。
 * 用 get_status 查看 ready=true 且 busy=false 后，再提交下一次绘图。
 * RGB565 是颜色存储格式，每个像素占两个字节；例如 0xFFFF 表示白色，0x0000 表示黑色。
 */
#ifndef LCD_H
#define LCD_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    bool ready;
    bool busy;
    uint32_t initialization_command_count;
    uint32_t completed_operation_count;
    uint32_t transmitted_pixel_count;
    uint32_t error_count;
} lcd_status_t;

bool lcd_init(uint32_t current_time_ms);
void lcd_process(uint32_t current_time_ms);
/* 左上角为 (0,0)，向右 x 增大，向下 y 增大，起点和终点都包含。
 * 例如 lcd_fill_rect(0, 0, 9, 9, 0xFFFF) 在左上角画 10x10 的白色方块。
 */
bool lcd_fill_rect(uint16_t x_start,
                   uint16_t y_start,
                   uint16_t x_end,
                   uint16_t y_end,
                   uint16_t color);
/* x/y 是图片左上角位置，width/height 是像素宽高，图片数据大小必须是 width*height*2 字节。
 * 像素按一行一行排列，每像素先放高字节、再放低字节；可参考 lcd_picture_data。
 * 函数返回后图片可能还没发完，正常完成（ready=true 且 busy=false）后才能修改或释放图片数组。
 */
bool lcd_draw_rgb565_image(uint16_t x,
                           uint16_t y,
                           uint16_t width,
                           uint16_t height,
                           const uint8_t *pixel_data,
                           uint32_t pixel_data_size);
bool lcd_get_status(lcd_status_t *status);

#endif
