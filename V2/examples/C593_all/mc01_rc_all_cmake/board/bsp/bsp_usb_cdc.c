#include "bsp_usb_cdc.h"

#include <string.h>

#include "ux_api.h"
#include "ux_device_class_cdc_acm.h"

#define BSP_USB_CDC_TX_BUFFER_SIZE    (64U)

static UX_SLAVE_CLASS_CDC_ACM *cdc_acm;
static uint8_t tx_buffer[BSP_USB_CDC_TX_BUFFER_SIZE];
static uint32_t tx_length;
static ULONG tx_actual_length;
static bool tx_pending;
static bool port_open;
static bsp_usb_cdc_tx_completion_t tx_completion;

void bsp_usb_cdc_on_activate(void *cdc_acm_instance)
{
    if (tx_pending)
    {
        tx_completion = BSP_USB_CDC_TX_COMPLETION_FAILED;
    }
    cdc_acm = (UX_SLAVE_CLASS_CDC_ACM *)cdc_acm_instance;
    tx_pending = false;
    port_open = false;
}

void bsp_usb_cdc_on_deactivate(void *cdc_acm_instance)
{
    if (cdc_acm == (UX_SLAVE_CLASS_CDC_ACM *)cdc_acm_instance)
    {
        cdc_acm = NULL;
    }
    if (tx_pending)
    {
        tx_completion = BSP_USB_CDC_TX_COMPLETION_FAILED;
    }
    tx_pending = false;
    port_open = false;
}

void bsp_usb_cdc_on_parameter_change(void *cdc_acm_instance)
{
    UX_SLAVE_CLASS_CDC_ACM_LINE_STATE_PARAMETER line_state;

    if ((cdc_acm == NULL) ||
        (cdc_acm != (UX_SLAVE_CLASS_CDC_ACM *)cdc_acm_instance) ||
        (ux_device_class_cdc_acm_ioctl(cdc_acm,
                                       UX_SLAVE_CLASS_CDC_ACM_IOCTL_GET_LINE_STATE,
                                       &line_state) != UX_SUCCESS))
    {
        port_open = false;
        return;
    }

    port_open = line_state.ux_slave_class_cdc_acm_parameter_dtr != 0U;
}

void bsp_usb_cdc_process(void)
{
    UINT state;

    if ((cdc_acm == NULL) || !tx_pending)
    {
        return;
    }

    state = ux_device_class_cdc_acm_write_run(cdc_acm, tx_buffer, tx_length, &tx_actual_length);
    if (state == UX_STATE_NEXT)
    {
        tx_pending = false;
        tx_completion = (tx_actual_length == (ULONG)tx_length) ?
                        BSP_USB_CDC_TX_COMPLETION_SUCCESS :
                        BSP_USB_CDC_TX_COMPLETION_FAILED;
    }
    else if ((state == UX_STATE_EXIT) || (state == UX_STATE_ERROR))
    {
        tx_pending = false;
        tx_completion = BSP_USB_CDC_TX_COMPLETION_FAILED;
    }
}

bool bsp_usb_cdc_is_connected(void)
{
    return cdc_acm != NULL;
}

bool bsp_usb_cdc_is_port_open(void)
{
    return port_open;
}

bool bsp_usb_cdc_tx_busy(void)
{
    return tx_pending;
}

bsp_usb_cdc_tx_completion_t bsp_usb_cdc_take_tx_completion(void)
{
    bsp_usb_cdc_tx_completion_t completion = tx_completion;

    tx_completion = BSP_USB_CDC_TX_COMPLETION_NONE;
    return completion;
}

bsp_usb_cdc_status_t bsp_usb_cdc_read(uint8_t *data, uint32_t capacity, uint32_t *actual_length)
{
    ULONG received_length = 0U;
    UINT state;

    if ((data == NULL) || (capacity == 0U) || (actual_length == NULL))
    {
        return BSP_USB_CDC_STATUS_INVALID_ARGUMENT;
    }

    *actual_length = 0U;
    if (cdc_acm == NULL)
    {
        return BSP_USB_CDC_STATUS_DISCONNECTED;
    }

    state = ux_device_class_cdc_acm_read_run(cdc_acm, data, capacity, &received_length);
    if (state == UX_STATE_NEXT)
    {
        *actual_length = (uint32_t)received_length;
        return BSP_USB_CDC_STATUS_OK;
    }
    if (state == UX_STATE_WAIT)
    {
        return BSP_USB_CDC_STATUS_BUSY;
    }
    return BSP_USB_CDC_STATUS_ERROR;
}

bsp_usb_cdc_status_t bsp_usb_cdc_write(const uint8_t *data, uint32_t length)
{
    if ((data == NULL) || (length == 0U) || (length > BSP_USB_CDC_TX_BUFFER_SIZE))
    {
        return BSP_USB_CDC_STATUS_INVALID_ARGUMENT;
    }
    if (cdc_acm == NULL)
    {
        return BSP_USB_CDC_STATUS_DISCONNECTED;
    }
    if (tx_pending)
    {
        return BSP_USB_CDC_STATUS_BUSY;
    }

    /* 先把待发内容复制保存，之后由 process 继续发送，所以原来的数组现在就可以再次使用。 */
    memcpy(tx_buffer, data, length);
    tx_length = length;
    tx_actual_length = 0U;
    tx_completion = BSP_USB_CDC_TX_COMPLETION_NONE;
    tx_pending = true;
    return BSP_USB_CDC_STATUS_OK;
}
