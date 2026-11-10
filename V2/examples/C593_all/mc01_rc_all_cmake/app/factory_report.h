/*
 * 在电脑串口工具里查看本例程的测试进度和最后一行中文结论。
 * 使用可传数据的 USB 线连接开发板，选择开发板对应的虚拟串口并打开，显示编码选 UTF-8。
 * 勾选串口工具的 DTR（打开信号）选项可请求报告；也可发送 REPORT 或 R，并附带回车或换行。
 * PASS 表示通过，FAIL 表示失败，RUN 表示还在测试，BLOCKED 表示所需的前一项失败而无法继续。
 * 自动通过后仍需实际观察灯光、听蜂鸣器并操作按键；LCD、SBUS 结果不计入这份自动报告。
 * 最终结果会保存下来，重复发送 REPORT 只是再看一遍，不会重新测试。
 */
#ifndef FACTORY_REPORT_H
#define FACTORY_REPORT_H

#include <stdint.h>

/* init 在程序启动时调用一次，process 放在主循环末尾反复调用，写法见 main.c。
 * USB 还需要 app_usbx_process 和 bsp_usb_cdc_process 一直运行，否则报告可能发不出来。
 * usb_test 也会使用同一个 USB 串口，不能与本报告功能同时运行。
 */
void factory_report_init(uint32_t current_time_ms);
void factory_report_process(uint32_t current_time_ms);

#endif
