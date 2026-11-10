/*
 * 这是单独的 SBUS 接收示例，用来把遥控器接收机的数据解释为各通道数值。
 * 程序先用预置数据检查自己的解析代码，再等待真实接收机送来数据；前一步通过不等于接线通过。
 * 当前 main 没有调用本模块，USB 报告也不统计 SBUS，不接 SBUS 不影响其他测试的启动。
 * 单独运行时先按开发板接口说明接入 SBUS 信号，再 init，并循环调用 process(HAL_GetTick())。
 */
#ifndef SBUS_TEST_H
#define SBUS_TEST_H

#include <stdint.h>

void sbus_test_init(uint32_t current_time_ms);
void sbus_test_process(uint32_t current_time_ms);

#endif
