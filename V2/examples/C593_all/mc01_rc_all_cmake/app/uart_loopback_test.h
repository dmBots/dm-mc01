/*
 * 检查串口发出的字节能否通过跳线原样回到自己，称为“回环测试”。
 * 断电后接两根跳线：P10 的 TX（PA9）接 RX（PA10）；P13 的 TX（PC10）接 RX（PC11）。
 * 这是每个串口各自连接自己的发送脚和接收脚，不是把 P10 与 P13 互连；不要接到电源脚。
 * 程序分别发送三种长度的数据，逐字节比较收到的内容；未接跳线会测试失败。
 * 调用方式见 main：init 做一次准备，process 放在 while(1) 中反复执行。
 */
#ifndef UART_LOOPBACK_TEST_H
#define UART_LOOPBACK_TEST_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    UART_TEST_RESULT_IDLE = 0U,
    UART_TEST_RESULT_RUNNING,
    UART_TEST_RESULT_PASSED,
    UART_TEST_RESULT_FAILED
} uart_test_result_t;

typedef struct
{
    uint32_t result;
    uint32_t passed_mask; /* 六个测试全部通过为 0x3F；前 3 位记录 USART1，后 3 位记录 USART3。 */
    uint32_t failure_code; /* 失败位置：例如 0x0102 表示第 1 个用例在编号为 2 的阶段失败。 */
    uint32_t last_rx_size;
    uint32_t usart1_error_count;
    uint32_t usart1_dropped_count;
    uint32_t usart3_error_count;
    uint32_t usart3_dropped_count;
} uart_loopback_test_summary_t;

void uart_loopback_test_init(uint32_t current_time_ms);
void uart_loopback_test_process(uint32_t current_time_ms);
/* 返回 true 只表示成功读取了状态；测试是否通过要看 summary->result，不能只看返回值。 */
bool uart_loopback_test_get_summary(uart_loopback_test_summary_t *summary);

#endif
