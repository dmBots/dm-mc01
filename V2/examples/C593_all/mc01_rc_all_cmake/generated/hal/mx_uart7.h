/**
  ******************************************************************************
  * @file           : mx_uart7.h
  * @brief          : Header for mx_uart7.c file.
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
#ifndef MX_UART7_H
#define MX_UART7_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32_hal.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/** Primary aliases for UART7_RX pin */
#define UART7_RX_PIN_PORT                     HAL_GPIOA
#define UART7_RX_PIN_PIN                      HAL_GPIO_PIN_8

/* Exported macros -----------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

/******************************************************************************/
/* Exported functions for UART in HAL layer */
/******************************************************************************/
/**
  * @brief mx_uart7_uart init function
  * This function configures the hardware resources used in this example
  * @retval pointer to handle or NULL in case of failure
  */
hal_uart_handle_t *mx_uart7_uart_init(void);

/**
  * @brief  De-initialize mx_uart7_uart instance and return it.
  * @retval None
  */
void mx_uart7_uart_deinit(void);

/**
  * @brief  Get the mx_uart7_uart object.
  * @retval Pointer on the mx_uart7_uartHandle
  */
hal_uart_handle_t *mx_uart7_uart_gethandle(void);

/******************************************************************************/
/*                           UART7 global interrupt                           */
/******************************************************************************/
void UART7_IRQHandler(void);

/******************************************************************************/
/*                      LPDMA2 channel0 global interrupt                      */
/******************************************************************************/
void LPDMA2_CH0_IRQHandler(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MX_UART7_H */
