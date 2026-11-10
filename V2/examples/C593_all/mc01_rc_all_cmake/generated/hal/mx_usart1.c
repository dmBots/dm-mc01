/**
  ******************************************************************************
  * @file           : mx_usart1.c
  * @brief          : USART1 Peripheral initialization
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
#include "mx_usart1.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions prototype------------------------------------------------*/
/* Exported variables by reference--------------------------------------------*/
/* Handle for UART */
static hal_uart_handle_t hUSART1;

static hal_dma_node_t DMA_Node_USART1_RX;
static hal_dma_handle_t hLPDMA1_CH4;
static hal_dma_handle_t hLPDMA1_CH5;

/* Exported function definition ----------------------------------------------*/
/******************************************************************************/
/* Exported functions for UART in HAL layer */
/******************************************************************************/

hal_uart_handle_t *mx_usart1_uart_init(void)
{
  hal_uart_config_t uart_config;

  /* Basic configuration */
  if (HAL_UART_Init(&hUSART1, HAL_UART1) != HAL_OK)
  {
    return NULL;
  }

  HAL_RCC_USART1_EnableClock();

  if (HAL_RCC_USART1_SetKernelClkSource(HAL_RCC_USART1_CLK_SRC_PCLK2) != HAL_OK)
  {
    return NULL;
  }

  uart_config.baud_rate = 115200;
  uart_config.clock_prescaler = HAL_UART_PRESCALER_DIV1;
  uart_config.word_length = HAL_UART_WORD_LENGTH_8_BIT;
  uart_config.stop_bits = HAL_UART_STOP_BIT_1;
  uart_config.parity = HAL_UART_PARITY_NONE;
  uart_config.direction = HAL_UART_DIRECTION_TX_RX;
  uart_config.hw_flow_ctl = HAL_UART_HW_CONTROL_NONE;
  uart_config.oversampling = HAL_UART_OVERSAMPLING_16;
  uart_config.one_bit_sampling = HAL_UART_ONE_BIT_SAMPLE_DISABLE;

  if (HAL_UART_SetConfig(&hUSART1, &uart_config) != HAL_OK)
  {
    return NULL;
  }

  /* Fifo configuration */
  if (HAL_UART_SetTxFifoThreshold(&hUSART1, HAL_UART_FIFO_THRESHOLD_1_8) != HAL_OK)
  {
    return NULL;
  }
  if (HAL_UART_SetRxFifoThreshold(&hUSART1, HAL_UART_FIFO_THRESHOLD_1_8) != HAL_OK)
  {
    return NULL;
  }
  if (HAL_UART_EnableFifoMode(&hUSART1) != HAL_OK)
  {
    return NULL;
  }

  if (HAL_UART_EnableReceiverTimeout(&hUSART1) != HAL_OK)
  {
    return NULL;
  }

  /* DMA stop on Rx error configuration */
  if (HAL_UART_EnableDMAStopOnRxError(&hUSART1) != HAL_OK)
  {
    return NULL;
  }

  /* ### USART1 GPIO Configuration ########################### */
  /* GPIO Clocks activation */
  HAL_RCC_GPIOA_EnableClock();

  hal_gpio_config_t  gpio_config;

  /**
    [GPIO Pin] ------> [Signal Name] ------> [Labels]

       PA10    ------>   USART1_RX   ------>  USART1_RX_PIN
       PA9     ------>   USART1_TX   ------>  USART1_TX_PIN
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
  gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.pull        = HAL_GPIO_PULL_NO;
  gpio_config.speed       = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.alternate   = HAL_GPIO_AF_7;
  HAL_GPIO_Init(HAL_GPIOA, USART1_RX_PIN_PIN | USART1_TX_PIN_PIN, &gpio_config);

  /* Enable interrupt */
  HAL_CORTEX_NVIC_SetPriority(USART1_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(USART1_IRQn);

  /* Configure the DMA TX */

  if (HAL_DMA_Init(&hLPDMA1_CH5, HAL_LPDMA1_CH5) != HAL_OK)
  {
    return NULL;
  }

  HAL_RCC_LPDMA1_EnableClock();

  hal_dma_direct_xfer_config_t xfer_cfg_usart1_tx_dma;
  xfer_cfg_usart1_tx_dma.request         = HAL_LPDMA1_REQUEST_USART1_TX;
  xfer_cfg_usart1_tx_dma.direction       = HAL_DMA_DIRECTION_MEMORY_TO_PERIPH;
  xfer_cfg_usart1_tx_dma.src_inc         = HAL_DMA_SRC_ADDR_INCREMENTED;
  xfer_cfg_usart1_tx_dma.dest_inc        = HAL_DMA_DEST_ADDR_FIXED;
  xfer_cfg_usart1_tx_dma.src_data_width  = HAL_DMA_SRC_DATA_WIDTH_BYTE;
  xfer_cfg_usart1_tx_dma.dest_data_width = HAL_DMA_DEST_DATA_WIDTH_BYTE;
  xfer_cfg_usart1_tx_dma.priority        = HAL_DMA_PRIORITY_LOW_WEIGHT_LOW;

  if (HAL_DMA_SetConfigPeriphDirectXfer(&hLPDMA1_CH5, &xfer_cfg_usart1_tx_dma) != HAL_OK)
  {
    return NULL;
  }

  /* Enable the interruption for LPDMA1_CH5 */
  HAL_CORTEX_NVIC_SetPriority(LPDMA1_CH5_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(LPDMA1_CH5_IRQn);

  /* Link the Transmit DMA handle to the UART handle */
  if (HAL_UART_SetTxDMA(&hUSART1, &hLPDMA1_CH5) != HAL_OK)
  {
    return NULL;
  }

  /* Configure the DMA RX */

  if (HAL_DMA_Init(&hLPDMA1_CH4, HAL_LPDMA1_CH4) != HAL_OK)
  {
    return NULL;
  }

  HAL_RCC_LPDMA1_EnableClock();

  hal_dma_direct_xfer_config_t xfer_cfg_usart1_rx_dma;
  xfer_cfg_usart1_rx_dma.request         = HAL_LPDMA1_REQUEST_USART1_RX;
  xfer_cfg_usart1_rx_dma.direction       = HAL_DMA_DIRECTION_PERIPH_TO_MEMORY;
  xfer_cfg_usart1_rx_dma.src_inc         = HAL_DMA_SRC_ADDR_FIXED;
  xfer_cfg_usart1_rx_dma.dest_inc        = HAL_DMA_DEST_ADDR_INCREMENTED;
  xfer_cfg_usart1_rx_dma.src_data_width  = HAL_DMA_SRC_DATA_WIDTH_BYTE;
  xfer_cfg_usart1_rx_dma.dest_data_width = HAL_DMA_DEST_DATA_WIDTH_BYTE;
  xfer_cfg_usart1_rx_dma.priority        = HAL_DMA_PRIORITY_LOW_WEIGHT_LOW;

  if (HAL_DMA_SetConfigPeriphLinkedListCircularXfer(&hLPDMA1_CH4, &DMA_Node_USART1_RX, &xfer_cfg_usart1_rx_dma) != HAL_OK)
  {
    return NULL;
  }

  /* Enable the interruption for LPDMA1_CH4 */
  HAL_CORTEX_NVIC_SetPriority(LPDMA1_CH4_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(LPDMA1_CH4_IRQn);

  /* Link the Receive DMA handle to the UART handle */
  if (HAL_UART_SetRxDMA(&hUSART1, &hLPDMA1_CH4) != HAL_OK)
  {
    return NULL;
  }

  return &hUSART1;
}

void mx_usart1_uart_deinit(void)
{
  /* Disable interrupt */
  HAL_CORTEX_NVIC_DisableIRQ(LPDMA1_CH4_IRQn);
(void)HAL_UART_DeInit(&hUSART1);

  HAL_RCC_USART1_Reset();

  HAL_RCC_USART1_DisableClock();

  /* De-initialize all GPIOA pins associated with USART1 */
  HAL_GPIO_DeInit(HAL_GPIOA, USART1_TX_PIN_PIN | USART1_RX_PIN_PIN);

  /* De-initialize the DMA channel */
  HAL_DMA_DeInit(&hLPDMA1_CH5);

  /* Disable the interruption for DMA */
  HAL_CORTEX_NVIC_DisableIRQ(LPDMA1_CH5_IRQn);

  /* De-initialize the DMA channel */
  HAL_DMA_DeInit(&hLPDMA1_CH4);

  /* Disable the interruption for DMA */
  HAL_CORTEX_NVIC_DisableIRQ(LPDMA1_CH4_IRQn);
}
hal_uart_handle_t *mx_usart1_uart_gethandle(void)
{
  return &hUSART1;
}

/******************************************************************************/
/*                          USART1 global interrupt                           */
/******************************************************************************/
void USART1_IRQHandler(void)
{
  HAL_UART_IRQHandler(&hUSART1);
}

/******************************************************************************/
/*                      LPDMA1 channel5 global interrupt                      */
/******************************************************************************/
void LPDMA1_CH5_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hLPDMA1_CH5);
}

/******************************************************************************/
/*                      LPDMA1 channel4 global interrupt                      */
/******************************************************************************/
void LPDMA1_CH4_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hLPDMA1_CH4);
}
