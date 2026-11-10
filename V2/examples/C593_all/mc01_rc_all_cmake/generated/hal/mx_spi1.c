/**
  ******************************************************************************
  * @file           : mx_spi1.c
  * @brief          : SPI1 Peripheral initialization
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the mx_stm32c5xx_hal_drivers_license.md file
  * in the same directory as the generated code.
  * If no mx_stm32c5xx_hal_drivers_license.md file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "mx_spi1.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions prototype------------------------------------------------*/
/* Exported variables by reference--------------------------------------------*/
static hal_spi_handle_t hSPI1;
static hal_dma_handle_t hLPDMA2_CH3;

/******************************************************************************/
/* Exported functions for SPI in HAL layer */
/******************************************************************************/
hal_spi_handle_t *mx_spi1_init(void)
{
  hal_spi_config_t spi_config;

  if (HAL_SPI_Init(&hSPI1, HAL_SPI1) != HAL_OK)
  {
    return NULL;
  }

  HAL_RCC_SPI1_EnableClock();

  if (HAL_RCC_SPI1_SetKernelClkSource(HAL_RCC_SPI1_CLK_SRC_PCLK2) != HAL_OK)
  {
    return NULL;
  }

  spi_config.mode = HAL_SPI_MODE_MASTER;
  spi_config.direction = HAL_SPI_DIRECTION_FULL_DUPLEX;
  spi_config.data_width = HAL_SPI_DATA_WIDTH_8_BIT;
  spi_config.clock_polarity = HAL_SPI_CLOCK_POLARITY_LOW;
  spi_config.clock_phase = HAL_SPI_CLOCK_PHASE_1_EDGE;
  spi_config.baud_rate_prescaler = HAL_SPI_BAUD_RATE_PRESCALER_16;
  spi_config.first_bit = HAL_SPI_MSB_FIRST;
  spi_config.nss_pin_management = HAL_SPI_NSS_PIN_MGMT_INTERNAL;

  if (HAL_SPI_SetConfig(&hSPI1, &spi_config) != HAL_OK)
  {
    return NULL;
  }

  /* ### SPI1 GPIO Configuration ########################### */
  /* GPIO Clocks activation */
  HAL_RCC_GPIOA_EnableClock();

  hal_gpio_config_t  gpio_config;

  /**
    [GPIO Pin] ------> [Signal Name] ------> [Labels]

       PA5     ------>   SPI1_SCK   ------>  SPI1_SCK_PIN
       PA6     ------>   SPI1_MISO   ------>  SPI1_MISO_PIN
       PA7     ------>   SPI1_MOSI   ------>  SPI1_MOSI_PIN
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
  gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.pull        = HAL_GPIO_PULL_NO;
  gpio_config.speed       = HAL_GPIO_SPEED_FREQ_HIGH;
  gpio_config.alternate   = HAL_GPIO_AF_5;
  HAL_GPIO_Init(HAL_GPIOA, SPI1_SCK_PIN_PIN | SPI1_MISO_PIN_PIN | SPI1_MOSI_PIN_PIN, &gpio_config);

  /* Configure the DMA TX */

  if (HAL_DMA_Init(&hLPDMA2_CH3, HAL_LPDMA2_CH3) != HAL_OK)
  {
    return NULL;
  }

  HAL_RCC_LPDMA2_EnableClock();

  hal_dma_direct_xfer_config_t xfer_cfg_spi1_tx_dma;
  xfer_cfg_spi1_tx_dma.request         = HAL_LPDMA2_REQUEST_SPI1_TX;
  xfer_cfg_spi1_tx_dma.direction       = HAL_DMA_DIRECTION_MEMORY_TO_PERIPH;
  xfer_cfg_spi1_tx_dma.src_inc         = HAL_DMA_SRC_ADDR_INCREMENTED;
  xfer_cfg_spi1_tx_dma.dest_inc        = HAL_DMA_DEST_ADDR_FIXED;
  xfer_cfg_spi1_tx_dma.src_data_width  = HAL_DMA_SRC_DATA_WIDTH_BYTE;
  xfer_cfg_spi1_tx_dma.dest_data_width = HAL_DMA_DEST_DATA_WIDTH_BYTE;
  xfer_cfg_spi1_tx_dma.priority        = HAL_DMA_PRIORITY_LOW_WEIGHT_LOW;

  if (HAL_DMA_SetConfigPeriphDirectXfer(&hLPDMA2_CH3, &xfer_cfg_spi1_tx_dma) != HAL_OK)
  {
    return NULL;
  }

  /* Enable the interruption for LPDMA2_CH3 */
  HAL_CORTEX_NVIC_SetPriority(LPDMA2_CH3_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(LPDMA2_CH3_IRQn);

  /* Link the Transmit DMA handle to the SPI handle */
  if (HAL_SPI_SetTxDMA(&hSPI1, &hLPDMA2_CH3) != HAL_OK)
  {
    return NULL;
  }

  /* Enable the interrupt for SPI */
  HAL_CORTEX_NVIC_SetPriority(SPI1_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(SPI1_IRQn);

  return &hSPI1;
}

void mx_spi1_deinit(void)
{
  /* Disable the interrupt for SPI */
  HAL_CORTEX_NVIC_DisableIRQ(SPI1_IRQn);

  (void)HAL_SPI_DeInit(&hSPI1);

  HAL_RCC_SPI1_Reset();

  HAL_RCC_SPI1_DisableClock();

  /* De-initialize all GPIOA pins associated with SPI1 */
  HAL_GPIO_DeInit(HAL_GPIOA, SPI1_SCK_PIN_PIN | SPI1_MISO_PIN_PIN | SPI1_MOSI_PIN_PIN);

  /* De-initialize the DMA channel */
  HAL_DMA_DeInit(&hLPDMA2_CH3);

  /* Disable the interruption for DMA */
  HAL_CORTEX_NVIC_DisableIRQ(LPDMA2_CH3_IRQn);
}

hal_spi_handle_t *mx_spi1_gethandle(void)
{
  return &hSPI1;
}

/******************************************************************************/
/*                      LPDMA2 channe3 global interrupt                       */
/******************************************************************************/
void LPDMA2_CH3_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hLPDMA2_CH3);
}

/******************************************************************************/
/*                           SPI1 global interrupt                            */
/******************************************************************************/
void SPI1_IRQHandler(void)
{
  HAL_SPI_IRQHandler(&hSPI1);
}
