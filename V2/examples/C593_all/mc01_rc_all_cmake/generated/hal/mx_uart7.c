/**
  ******************************************************************************
  * @file           : mx_uart7.c
  * @brief          : UART7 Peripheral initialization
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
#include "mx_uart7.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions prototype------------------------------------------------*/
/* Exported variables by reference--------------------------------------------*/
/* Handle for UART */
static hal_uart_handle_t hUART7;

static hal_dma_node_t DMA_Node_UART7_RX;
static hal_dma_handle_t hLPDMA2_CH0;

/* Exported function definition ----------------------------------------------*/
/******************************************************************************/
/* Exported functions for UART in HAL layer */
/******************************************************************************/

hal_uart_handle_t *mx_uart7_uart_init(void)
{
  hal_uart_config_t uart_config;

  /* Basic configuration */
  if (HAL_UART_Init(&hUART7, HAL_UART7) != HAL_OK)
  {
    return NULL;
  }

  HAL_RCC_UART7_EnableClock();

  if (HAL_RCC_UART7_SetKernelClkSource(HAL_RCC_UART7_CLK_SRC_PCLK1) != HAL_OK)
  {
    return NULL;
  }

  uart_config.baud_rate = 100000;
  uart_config.clock_prescaler = HAL_UART_PRESCALER_DIV1;
  uart_config.word_length = HAL_UART_WORD_LENGTH_9_BIT;
  uart_config.stop_bits = HAL_UART_STOP_BIT_2;
  uart_config.parity = HAL_UART_PARITY_EVEN;
  uart_config.direction = HAL_UART_DIRECTION_RX;
  uart_config.hw_flow_ctl = HAL_UART_HW_CONTROL_NONE;
  uart_config.oversampling = HAL_UART_OVERSAMPLING_16;
  uart_config.one_bit_sampling = HAL_UART_ONE_BIT_SAMPLE_DISABLE;

  if (HAL_UART_SetConfig(&hUART7, &uart_config) != HAL_OK)
  {
    return NULL;
  }

  /* Fifo configuration */
  if (HAL_UART_SetTxFifoThreshold(&hUART7, HAL_UART_FIFO_THRESHOLD_1_8) != HAL_OK)
  {
    return NULL;
  }
  if (HAL_UART_SetRxFifoThreshold(&hUART7, HAL_UART_FIFO_THRESHOLD_1_8) != HAL_OK)
  {
    return NULL;
  }
  if (HAL_UART_EnableFifoMode(&hUART7) != HAL_OK)
  {
    return NULL;
  }

  /* DMA stop on Rx error configuration */
  if (HAL_UART_EnableDMAStopOnRxError(&hUART7) != HAL_OK)
  {
    return NULL;
  }

  /* ### UART7 GPIO Configuration ########################### */
  /* GPIO Clocks activation */
  HAL_RCC_GPIOA_EnableClock();

  hal_gpio_config_t  gpio_config;

  /**
    [GPIO Pin] ------> [Signal Name] ------> [Labels]

       PA8     ------>   UART7_RX   ------>  UART7_RX_PIN
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
  gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.pull        = HAL_GPIO_PULL_NO;
  gpio_config.speed       = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.alternate   = HAL_GPIO_AF_10;
  HAL_GPIO_Init(UART7_RX_PIN_PORT, UART7_RX_PIN_PIN, &gpio_config);

  /* Enable interrupt */
  HAL_CORTEX_NVIC_SetPriority(UART7_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(UART7_IRQn);

  /* Configure the DMA RX */

  if (HAL_DMA_Init(&hLPDMA2_CH0, HAL_LPDMA2_CH0) != HAL_OK)
  {
    return NULL;
  }

  HAL_RCC_LPDMA2_EnableClock();

  hal_dma_direct_xfer_config_t xfer_cfg_uart7_rx_dma;
  xfer_cfg_uart7_rx_dma.request         = HAL_LPDMA2_REQUEST_UART7_RX;
  xfer_cfg_uart7_rx_dma.direction       = HAL_DMA_DIRECTION_PERIPH_TO_MEMORY;
  xfer_cfg_uart7_rx_dma.src_inc         = HAL_DMA_SRC_ADDR_FIXED;
  xfer_cfg_uart7_rx_dma.dest_inc        = HAL_DMA_DEST_ADDR_INCREMENTED;
  xfer_cfg_uart7_rx_dma.src_data_width  = HAL_DMA_SRC_DATA_WIDTH_BYTE;
  xfer_cfg_uart7_rx_dma.dest_data_width = HAL_DMA_DEST_DATA_WIDTH_BYTE;
  xfer_cfg_uart7_rx_dma.priority        = HAL_DMA_PRIORITY_LOW_WEIGHT_LOW;

  if (HAL_DMA_SetConfigPeriphLinkedListCircularXfer(&hLPDMA2_CH0, &DMA_Node_UART7_RX, &xfer_cfg_uart7_rx_dma) != HAL_OK)
  {
    return NULL;
  }

  /* Enable the interruption for LPDMA2_CH0 */
  HAL_CORTEX_NVIC_SetPriority(LPDMA2_CH0_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(LPDMA2_CH0_IRQn);

  /* Link the Receive DMA handle to the UART handle */
  if (HAL_UART_SetRxDMA(&hUART7, &hLPDMA2_CH0) != HAL_OK)
  {
    return NULL;
  }

  return &hUART7;
}

void mx_uart7_uart_deinit(void)
{
  /* Disable interrupt */
  HAL_CORTEX_NVIC_DisableIRQ(LPDMA2_CH0_IRQn);
(void)HAL_UART_DeInit(&hUART7);

  HAL_RCC_UART7_Reset();

  HAL_RCC_UART7_DisableClock();

  /* De-initialize all GPIOA pins associated with UART7 */
  HAL_GPIO_DeInit(UART7_RX_PIN_PORT, UART7_RX_PIN_PIN);

  /* De-initialize the DMA channel */
  HAL_DMA_DeInit(&hLPDMA2_CH0);

  /* Disable the interruption for DMA */
  HAL_CORTEX_NVIC_DisableIRQ(LPDMA2_CH0_IRQn);
}
hal_uart_handle_t *mx_uart7_uart_gethandle(void)
{
  return &hUART7;
}

/******************************************************************************/
/*                           UART7 global interrupt                           */
/******************************************************************************/
void UART7_IRQHandler(void)
{
  HAL_UART_IRQHandler(&hUART7);
}

/******************************************************************************/
/*                      LPDMA2 channel0 global interrupt                      */
/******************************************************************************/
void LPDMA2_CH0_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hLPDMA2_CH0);
}
