/*
 * 检查两路 RS485 接口能否把数据发过去，再原样送回来。
 * 断电后接测试线：P5 的 A 接 P4 的 A，B 接 B，GND 接 GND；先核对引脚标注，不要短接电源脚。
 * 程序先让一路发送，另一路收到后回送，再交换两路的角色；会测试不同长度的数据。
 * 接好后重新运行例程；未接线、接错线或返回数据不一致，测试都会失败。
 * 调用方式见 main：init 做一次准备，process 放在 while(1) 中反复执行。
 */
#ifndef RS485_ECHO_TEST_H
#define RS485_ECHO_TEST_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    RS485_TEST_RESULT_IDLE = 0U,
    RS485_TEST_RESULT_RUNNING,
    RS485_TEST_RESULT_PASSED,
    RS485_TEST_RESULT_FAILED
} rs485_test_result_t;

typedef struct
{
    uint32_t result;
    uint32_t passed_mask; /* 六个测试各占一位，全部通过为 0x3F；具体顺序见 rs485_echo_test.c 的 test_cases。 */
    uint32_t failure_code; /* 失败位置：例如 0x0104 表示第 1 个用例在编号为 4 的阶段失败。 */
    uint32_t last_rx_size;
    uint32_t port1_error_count;
    uint32_t port1_dropped_count;
    uint32_t port2_error_count;
    uint32_t port2_dropped_count;
} rs485_echo_test_summary_t;

void rs485_echo_test_init(uint32_t current_time_ms);
void rs485_echo_test_process(uint32_t current_time_ms);
/* 返回 true 只表示成功读取了状态；测试是否通过要看 summary->result，不能只看返回值。 */
bool rs485_echo_test_get_summary(rs485_echo_test_summary_t *summary);

#endif
