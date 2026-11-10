/**
  ******************************************************************************
  * @file           : mx_fdcan2.h
  * @brief          : Header for mx_fdcan2.c file.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef MX_FDCAN2_H
#define MX_FDCAN2_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32_hal.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/** Primary aliases for FDCAN2_TX pin */
#define FDCAN2_TX_PIN_PORT                    HAL_GPIOB
#define FDCAN2_TX_PIN_PIN                     HAL_GPIO_PIN_13

/** Primary aliases for FDCAN2_RX pin */
#define FDCAN2_RX_PIN_PORT                    HAL_GPIOB
#define FDCAN2_RX_PIN_PIN                     HAL_GPIO_PIN_12

/* Exported macros -----------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/******************************************************************************/
/* Exported functions for FDCAN in HAL layer */
/******************************************************************************/
/**
  * @brief mx_fdcan2 init function
  * This function configures the hardware resources used in this example
  * @retval pointer to handle or NULL in case of failure
  */
hal_fdcan_handle_t *mx_fdcan2_init(void);

/**
  * @brief  De-initialize fdcan2 instance and return it.
  */
void mx_fdcan2_deinit(void);

/**
  * @brief  Get the FDCAN2 object.
  * @retval Pointer on the FDCAN2 Handle
  */
hal_fdcan_handle_t *mx_fdcan2_gethandle(void);

/******************************************************************************/
/*                             FDCAN2 Interrupt 0                             */
/******************************************************************************/
void FDCAN2_IT0_IRQHandler(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MX_FDCAN2_H */
