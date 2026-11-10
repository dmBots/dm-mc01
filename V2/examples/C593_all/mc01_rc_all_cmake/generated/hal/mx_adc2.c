/**
  ******************************************************************************
  * @file           : mx_adc2.c
  * @brief          : ADC2 Peripheral initialization
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
#include "mx_adc2.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions prototype------------------------------------------------*/
/* Exported variables by reference--------------------------------------------*/
static hal_adc_handle_t hADC2;
static hal_dma_handle_t hLPDMA2_CH2;

/******************************************************************************/
/* Exported functions for ADC2 in HAL layer */
/******************************************************************************/
hal_adc_handle_t *mx_adc2_init(void)
{
  if (HAL_ADC_Init(&hADC2, HAL_ADC2) != HAL_OK)
  {
    return NULL;
  }

  HAL_RCC_ADC12_EnableClock();

  if (HAL_RCC_ADCDAC_SetKernelClkSource(HAL_RCC_ADCDAC_CLK_SRC_PSIS) != HAL_OK)
  {
    return NULL;
  }

  hal_adc_config_t adc_config;
  hal_adc_channel_config_t adc_channel_config;

  adc_config.resolution          = HAL_ADC_RESOLUTION_12_BIT;
  adc_config.sampling_mode       = HAL_ADC_SAMPLING_MODE_NORMAL;
  HAL_ADC_SetConfig(&hADC2, &adc_config);

  /****************************************************************************/
  /* Configuration of basic features (mandatory)                              */
  /****************************************************************************/

/* ==================== Group Regular ====================*/
  hal_adc_reg_config_t reg_config;
  reg_config.trigger_src        = HAL_ADC_REG_TRIG_SOFTWARE;
  reg_config.sequencer_length   = 1;
  reg_config.sequencer_discont  = HAL_ADC_REG_SEQ_DISCONT_DISABLE;
  reg_config.continuous         = HAL_ADC_REG_CONV_SINGLE;
  reg_config.overrun            = HAL_ADC_REG_OVR_DATA_OVERWRITTEN;
  HAL_ADC_REG_SetConfig(&hADC2, &reg_config);

  adc_channel_config.group           = HAL_ADC_GROUP_REGULAR;
  adc_channel_config.sequencer_rank  = 1;
  adc_channel_config.sampling_time   = HAL_ADC_SAMPLING_TIME_289CYCLES;
  adc_channel_config.input_mode      = HAL_ADC_IN_SINGLE_ENDED;
  HAL_ADC_SetConfigChannel(&hADC2, HAL_ADC_CHANNEL_4, &adc_channel_config);

  /****************************************************************************/
  /* Configuration of additional features (optional)                          */
  /****************************************************************************/

  /* ### ADC2 GPIO Configuration ########################### */
  /* GPIO Clocks activation */
  HAL_RCC_GPIOC_EnableClock();

  hal_gpio_config_t  gpio_config;

  /**
    [GPIO Pin] ------> [Signal Name] ------> [Labels]

       PC4     ------>   ADC2_IN4   ------>  ADC2_CH4_KEY_PIN
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ANALOG;
  gpio_config.pull        = HAL_GPIO_PULL_NO;
  HAL_GPIO_Init(ADC2_CH4_KEY_PIN_PORT, ADC2_CH4_KEY_PIN_PIN, &gpio_config);

  /* Enable the interruption for ADC */
  HAL_CORTEX_NVIC_SetPriority(ADC2_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(ADC2_IRQn);
  /* Configure DMA */

  if (HAL_DMA_Init(&hLPDMA2_CH2, HAL_LPDMA2_CH2) != HAL_OK)
  {
    return NULL;
  }

  HAL_RCC_LPDMA2_EnableClock();

  hal_dma_direct_xfer_config_t xfer_cfg_adc2_dma;
  xfer_cfg_adc2_dma.request         = HAL_LPDMA2_REQUEST_ADC2;
  xfer_cfg_adc2_dma.direction       = HAL_DMA_DIRECTION_PERIPH_TO_MEMORY;
  xfer_cfg_adc2_dma.src_inc         = HAL_DMA_SRC_ADDR_FIXED;
  xfer_cfg_adc2_dma.dest_inc        = HAL_DMA_DEST_ADDR_INCREMENTED;
  xfer_cfg_adc2_dma.src_data_width  = HAL_DMA_SRC_DATA_WIDTH_WORD;
  xfer_cfg_adc2_dma.dest_data_width = HAL_DMA_DEST_DATA_WIDTH_WORD;
  xfer_cfg_adc2_dma.priority        = HAL_DMA_PRIORITY_LOW_WEIGHT_LOW;

  if (HAL_DMA_SetConfigPeriphDirectXfer(&hLPDMA2_CH2, &xfer_cfg_adc2_dma) != HAL_OK)
  {
    return NULL;
  }

  /* Enable the interruption for LPDMA2_CH2 */
  HAL_CORTEX_NVIC_SetPriority(LPDMA2_CH2_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(LPDMA2_CH2_IRQn);

  /* Link DMA handle to ADC handle */
  HAL_ADC_REG_SetDMA(&hADC2, &hLPDMA2_CH2);

  return &hADC2;
}

void mx_adc2_deinit(void)
{
  (void)HAL_ADC_DeInit(&hADC2);

  /* Disable the interruption for ADC */
  HAL_CORTEX_NVIC_DisableIRQ(ADC2_IRQn);

  /* De-initialize all GPIOC pins associated with ADC2 */
  HAL_GPIO_DeInit(ADC2_CH4_KEY_PIN_PORT, ADC2_CH4_KEY_PIN_PIN);

  /* De-initialize the DMA channel */
  HAL_DMA_DeInit(&hLPDMA2_CH2);

  /* Disable the interruption for DMA */
  HAL_CORTEX_NVIC_DisableIRQ(LPDMA2_CH2_IRQn);
}

hal_adc_handle_t *mx_adc2_gethandle(void)
{
  return &hADC2;
}

/******************************************************************************/
/*                           ADC2 global interrupt                            */
/******************************************************************************/
void ADC2_IRQHandler(void)
{
  HAL_ADC_IRQHandler(&hADC2);
}

/******************************************************************************/
/*                      LPDMA2 channel2 global interrupt                      */
/******************************************************************************/
void LPDMA2_CH2_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hLPDMA2_CH2);
}
