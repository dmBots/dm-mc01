/*
 * 内置的 80x76 像素演示图片，每个像素用两个字节表示颜色，先放高字节再放低字节。
 * 可把 lcd_picture_data 交给 lcd_draw_rgb565_image 显示，完整调用示例见 lcd_test.c。
 */
#ifndef LCD_PICTURE_H
#define LCD_PICTURE_H

#include <stdint.h>

#define LCD_PICTURE_WIDTH      (80U)
#define LCD_PICTURE_HEIGHT     (76U)
#define LCD_PICTURE_DATA_SIZE  (LCD_PICTURE_WIDTH * LCD_PICTURE_HEIGHT * 2U)

extern const uint8_t lcd_picture_data[LCD_PICTURE_DATA_SIZE];

#endif
