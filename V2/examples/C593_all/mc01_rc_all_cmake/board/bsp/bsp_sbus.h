/*
 * 通过 UART7 串口接收 SBUS 数据，SBUS 常用于接收遥控器接收机的通道信息。
 * 这里仅接收字节；把一串字节解释为通道数值的代码在 app/sbus_test.c。
 * 当前综合例程没有调用 SBUS 测试，不需要为运行其他示例而接入 SBUS 信号。
 */
#ifndef BSP_SBUS_H
#define BSP_SBUS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    bool receiving;
    uint32_t rx_pending;
    uint32_t rx_dropped_count;
    uint32_t error_count;
    uint32_t rx_half_event_count;
    uint32_t rx_full_event_count;
    uint32_t restart_count;
} bsp_sbus_status_t;

/* start 开始接收；串口通信格式已由 CubeMX 生成代码设置，不在这里重新配置。 */
bool bsp_sbus_start(void);
/* 在主循环中反复调用；接收出错后，程序会在这里尝试恢复接收。 */
void bsp_sbus_process(void);
/* 取出收到的字节并返回字节数；一次可能只取到半条消息，需要上层拼接解析。 */
uint32_t bsp_sbus_read(uint8_t *data, uint32_t capacity);
uint32_t bsp_sbus_rx_pending(void);
bool bsp_sbus_get_status(bsp_sbus_status_t *status);
bool bsp_sbus_stop(void);

#endif
