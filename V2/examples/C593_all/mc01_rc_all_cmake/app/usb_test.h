/*
 * 这是单独的 USB 通信演示，不是当前综合例程正在输出的测试报告。
 * BOARD_USB_TEST_MODE 选择两种演示：原样发回电脑发送的数据，或向 VOFA 绘图工具发送正弦波数据。
 * VOFA 模式需要电脑工具使用 JustFloat 格式解析，普通文本串口工具不能把它直接显示成数字。
 * 当前 main 没有调用本模块；切换到它时应停用 factory_report，避免两者争用同一个 USB 串口。
 * 调用方式可参考 main 的 USB 初始化和循环处理，再接入本模块的 init/process。
 */
#ifndef USB_TEST_H
#define USB_TEST_H

#include <stdint.h>

void usb_test_init(uint32_t current_time_ms);
void usb_test_process(uint32_t current_time_ms);

#endif
