/*
 * 检查开发板能否发出周期脉冲并通过跳线收到，称为 PWM 回环测试。
 * 断电后接两根跳线：PC6（TIM8_CH1）接 PC7（TIM8_CH2），PB5（TIM8_CH3）接 PC9（TIM8_CH4）。
 * PC6/PB5 发脉冲，PC7/PC9 统计收到多少次；没接跳线时收不到脉冲，测试会失败。
 * init 调用一次，再循环调用 process(HAL_GetTick())，测完后用 get_summary 读结果。
 * 这里只检查收到的脉冲数量，不代表已经测量了输出电压或高低电平的时间比例。
 */
#ifndef TIM8_LOOPBACK_TEST_H
#define TIM8_LOOPBACK_TEST_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    TIM8_LOOPBACK_RESULT_IDLE = 0U,
    TIM8_LOOPBACK_RESULT_RUNNING,
    TIM8_LOOPBACK_RESULT_PASSED,
    TIM8_LOOPBACK_RESULT_FAILED
} tim8_loopback_result_t;

typedef enum
{
    TIM8_LOOPBACK_FAILURE_NONE = 0U,
    TIM8_LOOPBACK_FAILURE_CH1_TO_CH2 = (1UL << 0U),
    TIM8_LOOPBACK_FAILURE_CH3_TO_CH4 = (1UL << 1U),
    TIM8_LOOPBACK_FAILURE_BSP = (1UL << 2U)
} tim8_loopback_failure_t;

typedef struct
{
    uint32_t result;
    uint32_t failure_mask;
    uint32_t elapsed_ms;
    uint32_t expected_edge_count;
    uint32_t ch2_edge_count;
    uint32_t ch4_edge_count;
    uint32_t ch2_last_capture_ticks;
    uint32_t ch4_last_capture_ticks;
} tim8_loopback_test_summary_t;

void tim8_loopback_test_init(uint32_t current_time_ms);
void tim8_loopback_test_process(uint32_t current_time_ms);
/* 返回 true 只表示成功读取了状态；测试是否通过要看 summary->result，不能只看返回值。 */
bool tim8_loopback_test_get_summary(tim8_loopback_test_summary_t *summary);

#endif
