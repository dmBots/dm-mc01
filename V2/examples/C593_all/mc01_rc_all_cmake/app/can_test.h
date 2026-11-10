/*
 * 检查两路 CAN 能否发送和接收消息。
 * 先在芯片内部把发送数据送回接收端（内部回环），再检查两路接口之间的实际通信。
 * 外部测试需把 CAN1 和 CAN2 接到同一条 CAN 总线，按接口标注连接 H-H、L-L。
 * 先调用一次 init，再在 main 的 while(1) 中反复调用 process(HAL_GetTick())。
 * 未接测试线时外部通信项可能失败，不能仅据此判断开发板损坏。
 */
#ifndef CAN_TEST_H
#define CAN_TEST_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    CAN_TEST_RESULT_IDLE = 0U,
    CAN_TEST_RESULT_RUNNING,
    CAN_TEST_RESULT_PASSED,
    CAN_TEST_RESULT_FAILED
} can_test_result_t;

typedef struct
{
    uint32_t result;
    uint32_t passed_mask; /* 每通过一项就记录一位；四项全部通过为 0x0F，顺序见 can_test.c。 */
    uint32_t failure_code;
    uint32_t last_rx_identifier;
    uint32_t can1_tx_error_count;
    uint32_t can1_rx_error_count;
    uint32_t can1_rx_dropped_count;
    uint32_t can2_tx_error_count;
    uint32_t can2_rx_error_count;
    uint32_t can2_rx_dropped_count;
} can_test_summary_t;

void can_test_init(uint32_t current_time_ms);
void can_test_process(uint32_t current_time_ms);
/* 返回 true 只表示成功读取了状态；测试是否通过要看 summary->result，不能只看返回值。 */
bool can_test_get_summary(can_test_summary_t *summary);

#ifdef __cplusplus
}
#endif

#endif
