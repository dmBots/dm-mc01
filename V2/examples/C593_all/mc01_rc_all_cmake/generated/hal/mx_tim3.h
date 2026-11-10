/**
  ******************************************************************************
  * @file           : mx_tim3.h
  * @brief          : Header for mx_tim3.c file.
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
#ifndef MX_TIM3_H
#define MX_TIM3_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32_hal.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/** Primary aliases for TIM3_CH3 pin */
#define TIM3_CH3_PIN_PORT                     HAL_GPIOB
#define TIM3_CH3_PIN_PIN                      HAL_GPIO_PIN_0

/* Exported macros -----------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/******************************************************************************/
/* Exported functions for TIM in HAL layer */
/******************************************************************************/
/**
  * @brief  mx_tim3 init function.
  *         This function configures the hardware resources used in this example.
  * @retval Pointer to handle
  * @retval NULL in case of failure
  */
hal_tim_handle_t *mx_tim3_init(void);

/**
  * @brief  De-initialize mx_tim3 instance and return it.
  */
void mx_tim3_deinit(void);

/**
  * @brief  Get the mx_tim3 object.
  * @return Pointer on the mx_tim3 Handle
  */
hal_tim_handle_t *mx_tim3_gethandle(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MX_TIM3_H */
