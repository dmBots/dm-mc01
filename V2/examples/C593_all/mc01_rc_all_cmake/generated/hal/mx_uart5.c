/**
  ******************************************************************************
  * @file           : mx_uart5.c
  * @brief          : UART5 Peripheral initialization
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
#include "mx_uart5.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions prototype------------------------------------------------*/
/* Exported variables by reference--------------------------------------------*/
/* Handle for UART */
static hal_uart_handle_t hUART5;

static hal_dma_node_t DMA_Node_UART5_RX;
static hal_dma_handle_t hLPDMA1_CH2;
static hal_dma_handle_t hLPDMA1_CH3;

/* Exported function definition ----------------------------------------------*/
/******************************************************************************/
/* Exported functions for UART in HAL layer */
/******************************************************************************/

hal_uart_handle_t *mx_uart5_uart_init(void)
{
  hal_uart_config_t uart_config;

  /* Basic configuration */
  if (HAL_UART_Init(&hUART5, HAL_UART5) != HAL_OK)
  {
    return NULL;
  }

  HAL_RCC_UART5_EnableClock();

  if (HAL_RCC_UART5_SetKernelClkSource(HAL_RCC_UART5_CLK_SRC_PCLK1) != HAL_OK)
  {
    return NULL;
  }

  uart_config.baud_rate = 2000000;
  uart_config.clock_prescaler = HAL_UART_PRESCALER_DIV1;
  uart_config.word_length = HAL_UART_WORD_LENGTH_8_BIT;
  uart_config.stop_bits = HAL_UART_STOP_BIT_1;
  uart_config.parity = HAL_UART_PARITY_NONE;
  uart_config.direction = HAL_UART_DIRECTION_TX_RX;
  uart_config.hw_flow_ctl = HAL_UART_HW_CONTROL_RTS;
  uart_config.oversampling = HAL_UART_OVERSAMPLING_16;
  uart_config.one_bit_sampling = HAL_UART_ONE_BIT_SAMPLE_DISABLE;

  if (HAL_UART_SetConfig(&hUART5, &uart_config) != HAL_OK)
  {
    return NULL;
  }

  /* RS485 driver enable configuration */
  hal_uart_rs485_config_t uart_rs485_config;
  uart_rs485_config.assertion_time_samples = 0;
  uart_rs485_config.deassertion_time_samples = 0;
  uart_rs485_config.polarity = HAL_UART_DE_POLARITY_HIGH;

  if (HAL_UART_SetConfigRS485Mode(&hUART5, &uart_rs485_config) != HAL_OK)
  {
    return NULL;
  }

  if (HAL_UART_EnableRS485Mode(&hUART5) != HAL_OK)
  {
    return NULL;
  }

  /* Fifo configuration */
  if (HAL_UART_SetTxFifoThreshold(&hUART5, HAL_UART_FIFO_THRESHOLD_1_8) != HAL_OK)
  {
    return NULL;
  }
  if (HAL_UART_SetRxFifoThreshold(&hUART5, HAL_UART_FIFO_THRESHOLD_1_8) != HAL_OK)
  {
    return NULL;
  }
  if (HAL_UART_EnableFifoMode(&hUART5) != HAL_OK)
  {
    return NULL;
  }

  if (HAL_UART_EnableReceiverTimeout(&hUART5) != HAL_OK)
  {
    return NULL;
  }

  if (HAL_UART_SetConfigReceiverTimeout(&hUART5, 150) != HAL_OK)
  {
    return NULL;
  }
  /* DMA stop on Rx error configuration */
  if (HAL_UART_EnableDMAStopOnRxError(&hUART5) != HAL_OK)
  {
    return NULL;
  }

  /* ### UART5 GPIO Configuration ########################### */
  /* GPIO Clocks activation */
  HAL_RCC_GPIOD_EnableClock();

  HAL_RCC_GPIOC_EnableClock();

  hal_gpio_config_t  gpio_config;

  /**
    [GPIO Pin] ------> [Signal Name]

       PD2     ------>   UART5_RX
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
  gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.pull        = HAL_GPIO_PULL_NO;
  gpio_config.speed       = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.alternate   = HAL_GPIO_AF_8;
  HAL_GPIO_Init(HAL_GPIOD, HAL_GPIO_PIN_2, &gpio_config);

  /**
    [GPIO Pin] ------> [Signal Name] ------> [Labels]

       PC12    ------>   UART5_TX   ------>  UART5_TX_PIN
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
  gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.pull        = HAL_GPIO_PULL_NO;
  gpio_config.speed       = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.alternate   = HAL_GPIO_AF_8;
  HAL_GPIO_Init(UART5_TX_PIN_PORT, UART5_TX_PIN_PIN, &gpio_config);

  /**
    [GPIO Pin] ------> [Signal Name] ------> [Labels]

       PC8     ------>   UART5_RTS   ------>  UART5_DE_PIN
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
  gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.pull        = HAL_GPIO_PULL_NO;
  gpio_config.speed       = HAL_GPIO_SPEED_FREQ_VERY_HIGH;
  gpio_config.alternate   = HAL_GPIO_AF_8;
  HAL_GPIO_Init(UART5_DE_PIN_PORT, UART5_DE_PIN_PIN, &gpio_config);

  /* Enable interrupt */
  HAL_CORTEX_NVIC_SetPriority(UART5_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(UART5_IRQn);

  /* Configure the DMA TX */

  if (HAL_DMA_Init(&hLPDMA1_CH3, HAL_LPDMA1_CH3) != HAL_OK)
  {
    return NULL;
  }

  HAL_RCC_LPDMA1_EnableClock();

  hal_dma_direct_xfer_config_t xfer_cfg_uart5_tx_dma;
  xfer_cfg_uart5_tx_dma.request         = HAL_LPDMA1_REQUEST_UART5_TX;
  xfer_cfg_uart5_tx_dma.direction       = HAL_DMA_DIRECTION_MEMORY_TO_PERIPH;
  xfer_cfg_uart5_tx_dma.src_inc         = HAL_DMA_SRC_ADDR_INCREMENTED;
  xfer_cfg_uart5_tx_dma.dest_inc        = HAL_DMA_DEST_ADDR_FIXED;
  xfer_cfg_uart5_tx_dma.src_data_width  = HAL_DMA_SRC_DATA_WIDTH_BYTE;
  xfer_cfg_uart5_tx_dma.dest_data_width = HAL_DMA_DEST_DATA_WIDTH_BYTE;
  xfer_cfg_uart5_tx_dma.priority        = HAL_DMA_PRIORITY_LOW_WEIGHT_LOW;

  if (HAL_DMA_SetConfigPeriphDirectXfer(&hLPDMA1_CH3, &xfer_cfg_uart5_tx_dma) != HAL_OK)
  {
    return NULL;
  }

  /* Enable the interruption for LPDMA1_CH3 */
  HAL_CORTEX_NVIC_SetPriority(LPDMA1_CH3_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(LPDMA1_CH3_IRQn);

  /* Link the Transmit DMA handle to the UART handle */
  if (HAL_UART_SetTxDMA(&hUART5, &hLPDMA1_CH3) != HAL_OK)
  {
    return NULL;
  }

  /* Configure the DMA RX */

  if (HAL_DMA_Init(&hLPDMA1_CH2, HAL_LPDMA1_CH2) != HAL_OK)
  {
    return NULL;
  }

  HAL_RCC_LPDMA1_EnableClock();

  hal_dma_direct_xfer_config_t xfer_cfg_uart5_rx_dma;
  xfer_cfg_uart5_rx_dma.request         = HAL_LPDMA1_REQUEST_UART5_RX;
  xfer_cfg_uart5_rx_dma.direction       = HAL_DMA_DIRECTION_PERIPH_TO_MEMORY;
  xfer_cfg_uart5_rx_dma.src_inc         = HAL_DMA_SRC_ADDR_FIXED;
  xfer_cfg_uart5_rx_dma.dest_inc        = HAL_DMA_DEST_ADDR_INCREMENTED;
  xfer_cfg_uart5_rx_dma.src_data_width  = HAL_DMA_SRC_DATA_WIDTH_BYTE;
  xfer_cfg_uart5_rx_dma.dest_data_width = HAL_DMA_DEST_DATA_WIDTH_BYTE;
  xfer_cfg_uart5_rx_dma.priority        = HAL_DMA_PRIORITY_LOW_WEIGHT_HIGH;

  if (HAL_DMA_SetConfigPeriphLinkedListCircularXfer(&hLPDMA1_CH2, &DMA_Node_UART5_RX, &xfer_cfg_uart5_rx_dma) != HAL_OK)
  {
    return NULL;
  }

  /* Enable the interruption for LPDMA1_CH2 */
  HAL_CORTEX_NVIC_SetPriority(LPDMA1_CH2_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(LPDMA1_CH2_IRQn);

  /* Link the Receive DMA handle to the UART handle */
  if (HAL_UART_SetRxDMA(&hUART5, &hLPDMA1_CH2) != HAL_OK)
  {
    return NULL;
  }

  return &hUART5;
}

void mx_uart5_uart_deinit(void)
{
  /* Disable interrupt */
  HAL_CORTEX_NVIC_DisableIRQ(LPDMA1_CH2_IRQn);
(void)HAL_UART_DeInit(&hUART5);

  HAL_RCC_UART5_Reset();

  HAL_RCC_UART5_DisableClock();

  /* De-initialize all GPIOD pins associated with UART5 */
  HAL_GPIO_DeInit(HAL_GPIOD, HAL_GPIO_PIN_2);

  /* De-initialize all GPIOC pins associated with UART5 */
  HAL_GPIO_DeInit(HAL_GPIOC, UART5_DE_PIN_PIN | UART5_TX_PIN_PIN);

  /* De-initialize the DMA channel */
  HAL_DMA_DeInit(&hLPDMA1_CH3);

  /* Disable the interruption for DMA */
  HAL_CORTEX_NVIC_DisableIRQ(LPDMA1_CH3_IRQn);

  /* De-initialize the DMA channel */
  HAL_DMA_DeInit(&hLPDMA1_CH2);

  /* Disable the interruption for DMA */
  HAL_CORTEX_NVIC_DisableIRQ(LPDMA1_CH2_IRQn);
}
hal_uart_handle_t *mx_uart5_uart_gethandle(void)
{
  return &hUART5;
}

/******************************************************************************/
/*                           UART5 global interrupt                           */
/******************************************************************************/
void UART5_IRQHandler(void)
{
  HAL_UART_IRQHandler(&hUART5);
}

/******************************************************************************/
/*                      LPDMA1 channel3 global interrupt                      */
/******************************************************************************/
void LPDMA1_CH3_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hLPDMA1_CH3);
}

/******************************************************************************/
/*                      LPDMA1 channel2 global interrupt                      */
/******************************************************************************/
void LPDMA1_CH2_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hLPDMA1_CH2);
}
