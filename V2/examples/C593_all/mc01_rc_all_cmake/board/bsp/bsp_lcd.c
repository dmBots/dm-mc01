#include "bsp_lcd.h"

#include <stddef.h>

#include "board_config.h"
#include "mx_spi1.h"

typedef struct
{
    bool initialized;
    volatile bool busy;
    volatile bool tx_complete_pending;
    volatile bool error_pending;
    bool result_pending;
    bool result_success;
    uint32_t active_size;
    uint32_t spi_kernel_clock_hz;
    uint32_t spi_sclk_hz;
    uint32_t dma_transfer_count;
    uint32_t transmitted_byte_count;
    uint32_t error_count;
} bsp_lcd_context_t;

static bsp_lcd_context_t lcd_context;

static uint32_t bsp_lcd_prescaler_divisor(hal_spi_baud_rate_prescaler_t prescaler)
{
    switch (prescaler)
    {
        case HAL_SPI_BAUD_RATE_PRESCALER_2:
            return 2U;
        case HAL_SPI_BAUD_RATE_PRESCALER_4:
            return 4U;
        case HAL_SPI_BAUD_RATE_PRESCALER_8:
            return 8U;
        case HAL_SPI_BAUD_RATE_PRESCALER_16:
            return 16U;
        case HAL_SPI_BAUD_RATE_PRESCALER_32:
            return 32U;
        case HAL_SPI_BAUD_RATE_PRESCALER_64:
            return 64U;
        case HAL_SPI_BAUD_RATE_PRESCALER_128:
            return 128U;
        case HAL_SPI_BAUD_RATE_PRESCALER_256:
            return 256U;
        default:
            return 0U;
    }
}

bool bsp_lcd_init(void)
{
    hal_spi_handle_t *handle = mx_spi1_gethandle();
    uint32_t divisor;

    HAL_GPIO_WritePin(BOARD_LCD_CS_GPIO_PORT, BOARD_LCD_CS_GPIO_PIN, HAL_GPIO_PIN_SET);
    HAL_GPIO_WritePin(BOARD_LCD_DC_GPIO_PORT, BOARD_LCD_DC_GPIO_PIN, HAL_GPIO_PIN_SET);
    HAL_GPIO_WritePin(BOARD_LCD_RESET_GPIO_PORT, BOARD_LCD_RESET_GPIO_PIN, HAL_GPIO_PIN_SET);
    HAL_GPIO_WritePin(BOARD_LCD_BACKLIGHT_GPIO_PORT, BOARD_LCD_BACKLIGHT_GPIO_PIN,
                      HAL_GPIO_PIN_RESET);

    if ((handle == NULL) || (HAL_SPI_GetState(handle) != HAL_SPI_STATE_IDLE))
    {
        return false;
    }

    divisor = bsp_lcd_prescaler_divisor(HAL_SPI_GetBaudRatePrescaler(handle));
    if (divisor == 0U)
    {
        return false;
    }

    lcd_context.initialized = true;
    lcd_context.busy = false;
    lcd_context.tx_complete_pending = false;
    lcd_context.error_pending = false;
    lcd_context.result_pending = false;
    lcd_context.result_success = false;
    lcd_context.active_size = 0U;
    lcd_context.spi_kernel_clock_hz = HAL_SPI_GetClockFreq(handle);
    lcd_context.spi_sclk_hz = lcd_context.spi_kernel_clock_hz / divisor;
    lcd_context.dma_transfer_count = 0U;
    lcd_context.transmitted_byte_count = 0U;
    lcd_context.error_count = 0U;
    return true;
}

void bsp_lcd_process(void)
{
    bool transfer_finished;
    bool transfer_succeeded;

    if (!lcd_context.initialized || !lcd_context.busy)
    {
        return;
    }

    transfer_finished = lcd_context.tx_complete_pending || lcd_context.error_pending;
    if (!transfer_finished)
    {
        return;
    }

    transfer_succeeded = lcd_context.tx_complete_pending && !lcd_context.error_pending;
    lcd_context.tx_complete_pending = false;
    lcd_context.error_pending = false;
    HAL_GPIO_WritePin(BOARD_LCD_CS_GPIO_PORT, BOARD_LCD_CS_GPIO_PIN, HAL_GPIO_PIN_SET);
    lcd_context.busy = false;
    lcd_context.result_success = transfer_succeeded;
    lcd_context.result_pending = true;
    if (transfer_succeeded)
    {
        lcd_context.dma_transfer_count++;
        lcd_context.transmitted_byte_count += lcd_context.active_size;
    }
    else
    {
        lcd_context.error_count++;
    }
    lcd_context.active_size = 0U;
}

void bsp_lcd_set_reset(bool asserted)
{
    HAL_GPIO_WritePin(BOARD_LCD_RESET_GPIO_PORT, BOARD_LCD_RESET_GPIO_PIN,
                      asserted ? HAL_GPIO_PIN_RESET : HAL_GPIO_PIN_SET);
}

void bsp_lcd_set_backlight(bool enabled)
{
    HAL_GPIO_WritePin(BOARD_LCD_BACKLIGHT_GPIO_PORT, BOARD_LCD_BACKLIGHT_GPIO_PIN,
                      enabled ? HAL_GPIO_PIN_SET : HAL_GPIO_PIN_RESET);
}

bool bsp_lcd_transmit_dma(bsp_lcd_transfer_type_t type,
                          const uint8_t *data,
                          uint32_t size)
{
    hal_spi_handle_t *handle = mx_spi1_gethandle();

    if (!lcd_context.initialized || lcd_context.busy || lcd_context.result_pending ||
        (data == NULL) || (size == 0U) || (size > UINT16_MAX) ||
        ((type != BSP_LCD_TRANSFER_COMMAND) && (type != BSP_LCD_TRANSFER_DATA)))
    {
        return false;
    }

    HAL_GPIO_WritePin(BOARD_LCD_DC_GPIO_PORT, BOARD_LCD_DC_GPIO_PIN,
                      (type == BSP_LCD_TRANSFER_DATA) ? HAL_GPIO_PIN_SET : HAL_GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BOARD_LCD_CS_GPIO_PORT, BOARD_LCD_CS_GPIO_PIN, HAL_GPIO_PIN_RESET);
    lcd_context.busy = true;
    lcd_context.active_size = size;
    if (HAL_SPI_Transmit_DMA(handle, data, size) != HAL_OK)
    {
        HAL_GPIO_WritePin(BOARD_LCD_CS_GPIO_PORT, BOARD_LCD_CS_GPIO_PIN, HAL_GPIO_PIN_SET);
        lcd_context.busy = false;
        lcd_context.active_size = 0U;
        lcd_context.error_count++;
        return false;
    }
    return true;
}

bool bsp_lcd_take_transfer_result(bool *success)
{
    if ((success == NULL) || !lcd_context.result_pending)
    {
        return false;
    }

    *success = lcd_context.result_success;
    lcd_context.result_pending = false;
    return true;
}

bool bsp_lcd_get_status(bsp_lcd_status_t *status)
{
    if ((status == NULL) || !lcd_context.initialized)
    {
        return false;
    }

    status->busy = lcd_context.busy;
    status->spi_kernel_clock_hz = lcd_context.spi_kernel_clock_hz;
    status->spi_sclk_hz = lcd_context.spi_sclk_hz;
    status->dma_transfer_count = lcd_context.dma_transfer_count;
    status->transmitted_byte_count = lcd_context.transmitted_byte_count;
    status->error_count = lcd_context.error_count;
    return true;
}

/* 芯片自动通知发送完成时这里只做标记；主循环中的 process 再结束本次传输并交出结果。 */
void HAL_SPI_TxCpltCallback(hal_spi_handle_t *handle)
{
    if (handle == mx_spi1_gethandle())
    {
        lcd_context.tx_complete_pending = true;
    }
}

void HAL_SPI_ErrorCallback(hal_spi_handle_t *handle)
{
    if (handle == mx_spi1_gethandle())
    {
        lcd_context.error_pending = true;
    }
}
