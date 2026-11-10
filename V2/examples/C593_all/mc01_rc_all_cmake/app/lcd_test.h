/*
 * 演示开发板屏幕的显示效果：先清屏，再画文字、边线、图片和模拟按键读数。
 * 运行后请观察显示是否正常，并操作模拟按键，看屏幕上的数字是否变化。
 * 数字来自 key_test，两个模块需要一起运行；调用方式已在 main 中接好。
 * 程序会持续更新读数，但 LCD 结果不计入 USB 的自动测试结论，需要人工观察。
 */
#ifndef LCD_TEST_H
#define LCD_TEST_H

#include <stdint.h>

void lcd_test_init(uint32_t current_time_ms);
void lcd_test_process(uint32_t current_time_ms);

#endif
