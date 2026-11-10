/*
 * 控制两路 CAN 接口收发消息，消息中包含编号和数据。
 * PORT_1 对应芯片的 FDCAN1，PORT_2 对应 FDCAN2；完整接线和测试方法见 app/can_test.h。
 */
#ifndef BSP_CANFD_H
#define BSP_CANFD_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    BSP_CANFD_PORT_1 = 0U,
    BSP_CANFD_PORT_2
} bsp_canfd_port_t;

typedef enum
{
    BSP_CANFD_PRESET_CLASSIC_1M = 0U,
    BSP_CANFD_PRESET_FD_5M,
    BSP_CANFD_PRESET_FD_8M
} bsp_canfd_preset_t;

typedef enum
{
    BSP_CANFD_MODE_NORMAL = 0U,
    BSP_CANFD_MODE_INTERNAL_LOOPBACK
} bsp_canfd_mode_t;

typedef struct
{
    uint32_t identifier; /* 消息编号；本发送接口支持 0~0x7FF 的标准 ID。 */
    uint32_t length;     /* 数据字节数；可选 0~8，CAN FD 还可选 12/16/20/24/32/48/64。 */
    bool fd_frame;
    bool bit_rate_switch;
    uint8_t data[64];
} bsp_canfd_message_t;

typedef struct
{
    uint32_t tx_error_count;
    uint32_t rx_error_count;
    uint32_t rx_dropped_count;
} bsp_canfd_error_counters_t;

/* 先 configure 设置工作方式，再 start 开始通信；改设置前先 stop 停止。
 * configure 会清除尚未读取的接收消息，不能在接收过程中随意重新配置。
 */
bool bsp_canfd_configure(bsp_canfd_port_t port, bsp_canfd_preset_t preset, bsp_canfd_mode_t mode);
bool bsp_canfd_start(bsp_canfd_port_t port);
bool bsp_canfd_stop(bsp_canfd_port_t port);
/* 返回 true 只表示芯片已接受发送请求，不能据此判断另一端收到了消息。 */
bool bsp_canfd_transmit(bsp_canfd_port_t port, const bsp_canfd_message_t *message);
/* 每次取出一条已收到的消息；没有消息或参数无效时返回 false，主循环可稍后再读。 */
bool bsp_canfd_receive(bsp_canfd_port_t port, bsp_canfd_message_t *message);
uint32_t bsp_canfd_rx_pending(bsp_canfd_port_t port);
bool bsp_canfd_get_error_counters(bsp_canfd_port_t port, bsp_canfd_error_counters_t *counters);

#ifdef __cplusplus
}
#endif

#endif
