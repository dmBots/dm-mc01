/*
 * 屏幕显示文字时使用的内置点阵，每一位表示一个像素是否点亮。
 * 提供英文字母、数字和常用符号（ASCII 32~126），以及按编号访问的四个内置汉字。
 * 这里只提供字形数据，不能直接把任意中文字符串传进去；字符或编号不支持时返回 NULL。
 * 绘制示例见 lcd_test.c；每行按字节存放，每字节从低位到高位对应从左到右的像素。
 */
#ifndef LCD_FONT_H
#define LCD_FONT_H

#include <stdint.h>

#define LCD_FONT_ASCII_FIRST          (32U)
#define LCD_FONT_ASCII_LAST           (126U)
#define LCD_FONT_ASCII24_WIDTH        (12U)
#define LCD_FONT_ASCII24_HEIGHT       (24U)
#define LCD_FONT_ASCII32_WIDTH        (16U)
#define LCD_FONT_ASCII32_HEIGHT       (32U)
#define LCD_FONT_CHINESE32_WIDTH      (32U)
#define LCD_FONT_CHINESE32_HEIGHT     (32U)
#define LCD_FONT_CHINESE32_COUNT      (4U)

const uint8_t *lcd_font_get_ascii24(uint8_t character);
const uint8_t *lcd_font_get_ascii32(uint8_t character);
const uint8_t *lcd_font_get_chinese32(uint32_t index);

#endif
