/**
  ******************************************************************************
  * @file           : mx_fdcan2.c
  * @brief          : FDCAN2 Peripheral initialization
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
#include "mx_fdcan2.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions prototype------------------------------------------------*/
/* Exported variables by reference--------------------------------------------*/
static hal_fdcan_handle_t hFDCAN2;
/******************************************************************************/
/* Exported functions for FDCAN2 in HAL layer                                 */
/******************************************************************************/
hal_fdcan_handle_t *mx_fdcan2_init(void)
{
  hal_fdcan_config_t fdcan_config;

  HAL_RCC_FDCAN_EnableClock();

  if (HAL_FDCAN_Init(&hFDCAN2, HAL_FDCAN2) != HAL_OK)
  {
    return NULL;
  }

  if (HAL_RCC_FDCAN_SetKernelClkSource(HAL_RCC_FDCAN_CLK_SRC_PSIS) != HAL_OK)
  {
    return NULL;
  }

  /* FDCAN configuration */

  /* FDCAN clock Divider                      = 1 */
  /* FDCAN clock frequency after prescaler    = 160.000 MHz */

  /* Nominal bitrate                          = 500.000 kbps */
  /* Nominal time quanta                      = 12.50 ns */
  /* Nominal sample point                     = 87.50 % */

  hal_fdcan_nominal_bit_timing_t fdcan_nominal_bit_timing;
  fdcan_nominal_bit_timing.nominal_prescaler  = 2;
  fdcan_nominal_bit_timing.nominal_time_seg1  = 139;
  fdcan_nominal_bit_timing.nominal_time_seg2  = 20;
  fdcan_nominal_bit_timing.nominal_jump_width = 1;

  /* Data bitrate                             = 1000.000 kbps */
  /* Data time quanta                         = 25.00 ns */
  /* Data sample point                        = 75.00 % */

  hal_fdcan_data_bit_timing_t fdcan_data_bit_timing;
  fdcan_data_bit_timing.data_prescaler        = 4;
  fdcan_data_bit_timing.data_time_seg1        = 29;
  fdcan_data_bit_timing.data_time_seg2        = 10;
  fdcan_data_bit_timing.data_jump_width       = 1;

  fdcan_config.nominal_bit_timing   = fdcan_nominal_bit_timing;
  fdcan_config.data_bit_timing      = fdcan_data_bit_timing;

  fdcan_config.frame_format         = HAL_FDCAN_FRAME_FORMAT_FD_BRS;
  fdcan_config.mode                 = HAL_FDCAN_MODE_NORMAL;
  fdcan_config.auto_retransmission  = HAL_FDCAN_AUTO_RETRANSMISSION_ENABLE;
  fdcan_config.transmit_pause       = HAL_FDCAN_TRANSMIT_PAUSE_DISABLE;
  fdcan_config.protocol_exception   = HAL_FDCAN_PROTOCOL_EXCEPTION_ENABLE;
  fdcan_config.std_filters_nbr      = 1U;
  fdcan_config.ext_filters_nbr      = 1U;
  fdcan_config.tx_fifo_queue_mode   = HAL_FDCAN_TX_MODE_FIFO;

  if (HAL_FDCAN_SetConfig(&hFDCAN2, &fdcan_config) != HAL_OK)
  {
    return NULL;
  }

  /* Standard filters: 1 of 1 are configured, the remaining(s) is(are) allocated but not configured */

  hal_fdcan_filter_t filter_cfg;

  /* Standard filter 0 */
  filter_cfg.filter_index    = 0U;
  filter_cfg.id_type         = HAL_FDCAN_ID_STANDARD;
  filter_cfg.filter_type     = HAL_FDCAN_FILTER_TYPE_RANGE;
  filter_cfg.filter_config   = HAL_FDCAN_FILTER_TO_RX_FIFO_0;
  filter_cfg.filter_id1      = 0x00;
  filter_cfg.filter_id2      = 0x7FF;

  if (HAL_FDCAN_SetFilter(&hFDCAN2, &filter_cfg) != HAL_OK)
  {
    return NULL;
  }

  /* Extended filters: 1 of 1 are configured, the remaining(s) is(are) allocated but not configured */

  /* Extended filter 0 */
  filter_cfg.filter_index    = 0U;
  filter_cfg.id_type         = HAL_FDCAN_ID_EXTENDED;
  filter_cfg.filter_type     = HAL_FDCAN_FILTER_TYPE_RANGE;
  filter_cfg.filter_config   = HAL_FDCAN_FILTER_TO_RX_FIFO_1;
  filter_cfg.filter_id1      = 0x00;
  filter_cfg.filter_id2      = 0x1FFFFFFF;

  if (HAL_FDCAN_SetFilter(&hFDCAN2, &filter_cfg) != HAL_OK)
  {
    return NULL;
  }

  /* Configure the global filter acceptance/rejection rules */
  hal_fdcan_global_filter_config_t global_filter_cfg;
  global_filter_cfg.acceptance_non_matching_std = HAL_FDCAN_NO_MATCH_TO_RX_FIFO_0;
  global_filter_cfg.acceptance_non_matching_ext = HAL_FDCAN_NO_MATCH_TO_RX_FIFO_0;
  global_filter_cfg.acceptance_remote_std       = HAL_FDCAN_REMOTE_REJECT;
  global_filter_cfg.acceptance_remote_ext       = HAL_FDCAN_REMOTE_REJECT;
  if (HAL_FDCAN_SetGlobalFilter(&hFDCAN2, &global_filter_cfg) != HAL_OK)
  {
    return NULL;
  }

  hal_fdcan_tx_delay_comp_config_t tx_delay_compensation_cfg;
  tx_delay_compensation_cfg.tx_delay_comp_offset      = 0;
  tx_delay_compensation_cfg.tx_delay_comp_win_length  = 0;

  if (HAL_FDCAN_SetConfigTxDelayCompensation(&hFDCAN2, &tx_delay_compensation_cfg) != HAL_OK)
  {
    return NULL;
  }

  /* ### FDCAN2 GPIO Configuration ########################### */
  /* GPIO Clocks activation */
  HAL_RCC_GPIOB_EnableClock();

  hal_gpio_config_t  gpio_config;

  /**
    [GPIO Pin] ------> [Signal Name] ------> [Labels]

       PB13    ------>   FDCAN2_TX   ------>  FDCAN2_TX_PIN
       PB12    ------>   FDCAN2_RX   ------>  FDCAN2_RX_PIN
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
  gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.pull        = HAL_GPIO_PULL_NO;
  gpio_config.speed       = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.alternate   = HAL_GPIO_AF_9;
  HAL_GPIO_Init(HAL_GPIOB, FDCAN2_TX_PIN_PIN | FDCAN2_RX_PIN_PIN, &gpio_config);

  /* Enable individual interrupts */
  if (HAL_FDCAN_EnableInterrupts(&hFDCAN2, HAL_FDCAN_IT_RX_FIFO_0_NEW_MSG
                                         | HAL_FDCAN_IT_RX_FIFO_1_NEW_MSG) != HAL_OK)
  {
    return NULL;
  }

  /* Connect interrupt groups to interrupt line_0 */
  if (HAL_FDCAN_SetInterruptGroupsToLine(&hFDCAN2,
                                         HAL_FDCAN_IT_GROUP_RX_FIFO_0
                                         | HAL_FDCAN_IT_GROUP_RX_FIFO_1,
                                         HAL_FDCAN_IT_LINE_0) != HAL_OK)
  {
    return NULL;
  }

  if (HAL_FDCAN_EnableInterruptLines(&hFDCAN2, HAL_FDCAN_IT_LINE_0) != HAL_OK)
  {
    return NULL;
  }

  HAL_CORTEX_NVIC_SetPriority(FDCAN2_IT0_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(FDCAN2_IT0_IRQn);

  return &hFDCAN2;
}

void mx_fdcan2_deinit(void)
{
  /* Disable the FDCAN interrupt line 0 */
  HAL_CORTEX_NVIC_DisableIRQ(FDCAN2_IT0_IRQn);

  /* Deinitialize the FDCAN peripheral */
  (void)HAL_FDCAN_DeInit(&hFDCAN2);

  /* De-initialize all GPIOB pins associated with FDCAN2 */
  HAL_GPIO_DeInit(HAL_GPIOB, FDCAN2_RX_PIN_PIN | FDCAN2_TX_PIN_PIN);
}

hal_fdcan_handle_t *mx_fdcan2_gethandle(void)
{
  return &hFDCAN2;
}

/******************************************************************************/
/*                             FDCAN2 Interrupt 0                             */
/******************************************************************************/
void FDCAN2_IT0_IRQHandler(void)
{
  HAL_FDCAN_Line0_IRQHandler(&hFDCAN2);
}
