/*
 * 把开发板的 USB 当作虚拟串口，与电脑串口工具收发数据，不需要另接 USB 转串口模块。
 * USBX 是 USB 通信底层；先调用 app_usbx_init，并在主循环持续调用 app_usbx_process。
 * 发送函数只先保存待发数据，还要反复调用 bsp_usb_cdc_process，数据才会继续发向电脑。
 */
#ifndef BSP_USB_CDC_H
#define BSP_USB_CDC_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    BSP_USB_CDC_STATUS_OK = 0U,
    BSP_USB_CDC_STATUS_BUSY,
    BSP_USB_CDC_STATUS_DISCONNECTED,
    BSP_USB_CDC_STATUS_INVALID_ARGUMENT,
    BSP_USB_CDC_STATUS_ERROR
} bsp_usb_cdc_status_t;

typedef enum
{
    BSP_USB_CDC_TX_COMPLETION_NONE = 0U,
    BSP_USB_CDC_TX_COMPLETION_SUCCESS,
    BSP_USB_CDC_TX_COMPLETION_FAILED
} bsp_usb_cdc_tx_completion_t;

/* 以下 on_* 在 USB 连接状态变化时由底层自动调用，使用示例时不需要手动调用。 */
void bsp_usb_cdc_on_activate(void *cdc_acm_instance);
void bsp_usb_cdc_on_deactivate(void *cdc_acm_instance);
void bsp_usb_cdc_on_parameter_change(void *cdc_acm_instance);
void bsp_usb_cdc_process(void);
bool bsp_usb_cdc_is_connected(void);
/* connected 表示 USB 串口功能已激活；port_open 还检查电脑是否发送了 DTR 打开信号。
 * DTR 是串口工具中的控制选项，本例程自动发送报告需要勾选它。
 */
bool bsp_usb_cdc_is_port_open(void);
bool bsp_usb_cdc_tx_busy(void);
/* 读取一次发送完成结果后，该结果会被清除；write 返回 OK 不代表数据已经发完。 */
bsp_usb_cdc_tx_completion_t bsp_usb_cdc_take_tx_completion(void);
/* 返回 BUSY 表示读取还没完成，稍后继续调用；返回 OK 时 actual_length 是收到的字节数。 */
bsp_usb_cdc_status_t bsp_usb_cdc_read(uint8_t *data, uint32_t capacity, uint32_t *actual_length);
/* 每次可发 1~64 字节；返回 OK 后数据已复制保存，原数组可以修改或再次使用。
 * 返回 BUSY 表示上一份数据还没发完，应稍后重试，不要一直卡在这里等待。
 */
bsp_usb_cdc_status_t bsp_usb_cdc_write(const uint8_t *data, uint32_t length);

#endif
