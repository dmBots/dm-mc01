#include "bsp_spi2.h"

#include <stddef.h>

#include "board_config.h"
#include "mx_spi2.h"

static bool bsp_spi2_get_chip_select(bsp_spi2_chip_select_t chip_select,
                                     hal_gpio_t *gpio_port,
                                     uint32_t *gpio_pin)
{
    if (chip_select == BSP_SPI2_CHIP_SELECT_0)
    {
        *gpio_port = BOARD_SPI2_CS0_GPIO_PORT;
        *gpio_pin = BOARD_SPI2_CS0_GPIO_PIN;
        return true;
    }
    if (chip_select == BSP_SPI2_CHIP_SELECT_1)
    {
        *gpio_port = BOARD_SPI2_CS1_GPIO_PORT;
        *gpio_pin = BOARD_SPI2_CS1_GPIO_PIN;
        return true;
    }
    return false;
}

bool bsp_spi2_init(void)
{
    hal_spi_handle_t *handle = mx_spi2_gethandle();

    HAL_GPIO_WritePin(BOARD_SPI2_CS0_GPIO_PORT, BOARD_SPI2_CS0_GPIO_PIN,
                      BOARD_SPI2_CS_INACTIVE_STATE);
    HAL_GPIO_WritePin(BOARD_SPI2_CS1_GPIO_PORT, BOARD_SPI2_CS1_GPIO_PIN,
                      BOARD_SPI2_CS_INACTIVE_STATE);

    if ((handle == NULL) ||
        (HAL_SPI_SetClockPolarity(handle, HAL_SPI_CLOCK_POLARITY_HIGH) != HAL_OK) ||
        (HAL_SPI_SetClockPhase(handle, HAL_SPI_CLOCK_PHASE_2_EDGE) != HAL_OK))
    {
        return false;
    }

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    return true;
}

bool bsp_spi2_transfer(bsp_spi2_chip_select_t chip_select,
                       const uint8_t *tx_data,
                       uint8_t *rx_data,
                       uint32_t size)
{
    hal_gpio_t gpio_port;
    uint32_t gpio_pin;
    hal_status_t status;

    if ((tx_data == NULL) || (rx_data == NULL) || (size == 0U) ||
        !bsp_spi2_get_chip_select(chip_select, &gpio_port, &gpio_pin))
    {
        return false;
    }

    HAL_GPIO_WritePin(gpio_port, gpio_pin, BOARD_SPI2_CS_ACTIVE_STATE);
    status = HAL_SPI_TransmitReceive(mx_spi2_gethandle(), tx_data, rx_data, size,
                                     BOARD_SPI2_TRANSFER_TIMEOUT_MS);
    HAL_GPIO_WritePin(gpio_port, gpio_pin, BOARD_SPI2_CS_INACTIVE_STATE);
    return status == HAL_OK;
}

void bsp_spi2_delay_us(uint32_t delay_us)
{
    uint32_t start_cycle = DWT->CYCCNT;
    uint32_t delay_cycles = delay_us * (SystemCoreClock / 1000000U);

    while ((DWT->CYCCNT - start_cycle) < delay_cycles)
    {
    }
}
