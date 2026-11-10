/**
  ******************************************************************************
  * @file           : mx_gpio_default.h
  * @brief          : Header for mx_gpio_default.c file.
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
#ifndef MX_GPIO_DEFAULT_H
#define MX_GPIO_DEFAULT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32_hal.h"
#include "mx_def.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/******************************************************************************/
/* Exported defines for gpio_default in HAL layer                             */
/******************************************************************************/

/* Primary aliases for GPIO PA4 pin */
#define SPI1_CS_PIN_PORT                                HAL_GPIOA
#define SPI1_CS_PIN_PIN                                 HAL_GPIO_PIN_4
#define SPI1_CS_PIN_INIT_STATE                          HAL_GPIO_PIN_SET
#define SPI1_CS_PIN_ACTIVE_STATE                        HAL_GPIO_PIN_SET
#define SPI1_CS_PIN_INACTIVE_STATE                      HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PA15(JTDI) pin */
#define USER_KEY_PIN_PORT                               HAL_GPIOA
#define USER_KEY_PIN_PIN                                HAL_GPIO_PIN_15

/* Primary aliases for GPIO PB1 pin */
#define SPI2_CS0_PIN_PORT                               HAL_GPIOB
#define SPI2_CS0_PIN_PIN                                HAL_GPIO_PIN_1
#define SPI2_CS0_PIN_INIT_STATE                         HAL_GPIO_PIN_SET
#define SPI2_CS0_PIN_ACTIVE_STATE                       HAL_GPIO_PIN_SET
#define SPI2_CS0_PIN_INACTIVE_STATE                     HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PB3(JTDO/TRACESWO) pin */
#define IIC2_SCL_PIN_PORT                               HAL_GPIOB
#define IIC2_SCL_PIN_PIN                                HAL_GPIO_PIN_3
#define IIC2_SCL_PIN_INIT_STATE                         HAL_GPIO_PIN_RESET
#define IIC2_SCL_PIN_ACTIVE_STATE                       HAL_GPIO_PIN_SET
#define IIC2_SCL_PIN_INACTIVE_STATE                     HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PB4(NJTRST) pin */
#define IIC2_SDA_PIN_PORT                               HAL_GPIOB
#define IIC2_SDA_PIN_PIN                                HAL_GPIO_PIN_4
#define IIC2_SDA_PIN_INIT_STATE                         HAL_GPIO_PIN_SET
#define IIC2_SDA_PIN_ACTIVE_STATE                       HAL_GPIO_PIN_SET
#define IIC2_SDA_PIN_INACTIVE_STATE                     HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PC0 pin */
#define USER_LED_PIN_PORT                               HAL_GPIOC
#define USER_LED_PIN_PIN                                HAL_GPIO_PIN_0
#define USER_LED_PIN_INIT_STATE                         HAL_GPIO_PIN_SET
#define USER_LED_PIN_ACTIVE_STATE                       HAL_GPIO_PIN_RESET
#define USER_LED_PIN_INACTIVE_STATE                     HAL_GPIO_PIN_SET

/* Primary aliases for GPIO PC5 pin */
#define SPI2_CS1_PIN_PORT                               HAL_GPIOC
#define SPI2_CS1_PIN_PIN                                HAL_GPIO_PIN_5
#define SPI2_CS1_PIN_INIT_STATE                         HAL_GPIO_PIN_SET
#define SPI2_CS1_PIN_ACTIVE_STATE                       HAL_GPIO_PIN_SET
#define SPI2_CS1_PIN_INACTIVE_STATE                     HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PC14-OSC32_IN(OSC32_IN) pin */
#define OUT_5V_EN_PIN_PORT                              HAL_GPIOC
#define OUT_5V_EN_PIN_PIN                               HAL_GPIO_PIN_14
#define OUT_5V_EN_PIN_INIT_STATE                        HAL_GPIO_PIN_RESET
#define OUT_5V_EN_PIN_ACTIVE_STATE                      HAL_GPIO_PIN_SET
#define OUT_5V_EN_PIN_INACTIVE_STATE                    HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PC15-OSC32_OUT(OSC32_OUT) pin */
#define HEAT_5V_EN_PIN_PORT                             HAL_GPIOC
#define HEAT_5V_EN_PIN_PIN                              HAL_GPIO_PIN_15
#define HEAT_5V_EN_PIN_INIT_STATE                       HAL_GPIO_PIN_RESET
#define HEAT_5V_EN_PIN_ACTIVE_STATE                     HAL_GPIO_PIN_SET
#define HEAT_5V_EN_PIN_INACTIVE_STATE                   HAL_GPIO_PIN_RESET

/* Exported macros -----------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/******************************************************************************/
/* Exported functions for gpio_default in HAL layer                           */
/******************************************************************************/
/**
  * @brief mx_gpio_default init function
  * This function configures the hardware resources used in this example
  * @retval 0  GPIO group correctly initialized
  * @retval -1 Issue detected during GPIO group initialization
  */
system_status_t mx_gpio_default_init(void);

/**
  * @brief  De-initialize gpio_default instance.
  */
system_status_t mx_gpio_default_deinit(void);

/**
  * @brief  Get the EXTI15 object.
  * @retval Pointer on the EXTI15 Handle
  */
hal_exti_handle_t *mx_gpio_default_exti15_gethandle(void);

/******************************************************************************/
/*                           EXTI Line15 interrupt                            */
/******************************************************************************/
void EXTI15_IRQHandler(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MX_GPIO_DEFAULT_H */
