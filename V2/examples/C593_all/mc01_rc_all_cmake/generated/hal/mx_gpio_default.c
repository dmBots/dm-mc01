/**
  ******************************************************************************
  * @file           : mx_gpio_default.c
  * @brief          : gpio_default Peripheral initialization
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
#include "mx_gpio_default.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/* Exported variables by reference -------------------------------------------*/
static hal_exti_handle_t hEXTI15;

/******************************************************************************/
/* Exported functions for GPIO in HAL layer                                   */
/******************************************************************************/
system_status_t mx_gpio_default_init(void)
{
  hal_gpio_config_t  gpio_config;

  HAL_RCC_GPIOA_EnableClock();

  HAL_RCC_GPIOB_EnableClock();

  HAL_RCC_GPIOC_EnableClock();

  /*
    GPIO pin labels :
    PA4   ---------> SPI1_CS_PIN
    */
  /* Configure PA4 GPIO pin in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_VERY_HIGH;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = SPI1_CS_PIN_INIT_STATE;
  if (HAL_GPIO_Init(SPI1_CS_PIN_PORT, SPI1_CS_PIN_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PA15  ---------> USER_KEY_PIN
    */
  /* Configure PA15 GPIO pin in input mode */
  gpio_config.mode            = HAL_GPIO_MODE_INPUT;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  if (HAL_GPIO_Init(USER_KEY_PIN_PORT, USER_KEY_PIN_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  hal_exti_config_t exti_config;

  /* Initialize the EXTI for line 15 */
  HAL_EXTI_Init(&hEXTI15, HAL_EXTI_LINE_15);

  /* Set the trigger as RISING_FALLING for the GPIOA */
  exti_config.trigger   = HAL_EXTI_TRIGGER_RISING_FALLING;
  exti_config.gpio_port = HAL_EXTI_GPIOA;
  HAL_EXTI_SetConfig(&hEXTI15, &exti_config);

  /* Enable the INTERRUPT mode */
  HAL_EXTI_Enable(&hEXTI15, HAL_EXTI_MODE_INTERRUPT);

  /* Set line 15 Interrupt priority */
  HAL_CORTEX_NVIC_SetPriority(EXTI15_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(EXTI15_IRQn);

  /*
    GPIO pin labels :
    PB1   ---------> SPI2_CS0_PIN
    */
  /* Configure PB1 GPIO pin in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_VERY_HIGH;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = SPI2_CS0_PIN_INIT_STATE;
  if (HAL_GPIO_Init(SPI2_CS0_PIN_PORT, SPI2_CS0_PIN_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PB3   ---------> IIC2_SCL_PIN
    */
  /* Configure PB3 GPIO pin in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_VERY_HIGH;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = IIC2_SCL_PIN_INIT_STATE;
  if (HAL_GPIO_Init(IIC2_SCL_PIN_PORT, IIC2_SCL_PIN_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PB4   ---------> IIC2_SDA_PIN
    */
  /* Configure PB4 GPIO pin in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_VERY_HIGH;
  gpio_config.pull            = HAL_GPIO_PULL_UP;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = IIC2_SDA_PIN_INIT_STATE;
  if (HAL_GPIO_Init(IIC2_SDA_PIN_PORT, IIC2_SDA_PIN_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PC0   ---------> USER_LED_PIN
    */
  /* Configure PC0 GPIO pin in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = USER_LED_PIN_INIT_STATE;
  if (HAL_GPIO_Init(USER_LED_PIN_PORT, USER_LED_PIN_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PC5   ---------> SPI2_CS1_PIN
    */
  /* Configure PC1, PC5 GPIO pins in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_VERY_HIGH;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = HAL_GPIO_PIN_SET;
  if (HAL_GPIO_Init(HAL_GPIOC, HAL_GPIO_PIN_1 | SPI2_CS1_PIN_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PC14  ---------> OUT_5V_EN_PIN
    PC15  ---------> HEAT_5V_EN_PIN
    */
  /* Configure PC14, PC15 GPIO pins in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = HAL_GPIO_PIN_RESET;
  if (HAL_GPIO_Init(HAL_GPIOC, OUT_5V_EN_PIN_PIN | HEAT_5V_EN_PIN_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  return SYSTEM_OK;
}

system_status_t mx_gpio_default_deinit(void)
{
  /* De-initialize the EXTI for GPIOA line15 */
  HAL_EXTI_DeInit(&hEXTI15);

  /* set line 15 Interrupt priority */
  HAL_CORTEX_NVIC_DisableIRQ(EXTI15_IRQn);

  /* De-initialize pins of GPIOA port */
  HAL_GPIO_DeInit(HAL_GPIOA, SPI1_CS_PIN_PIN | USER_KEY_PIN_PIN);

  /* De-initialize pins of GPIOB port */
  HAL_GPIO_DeInit(HAL_GPIOB, SPI2_CS0_PIN_PIN | IIC2_SCL_PIN_PIN | IIC2_SDA_PIN_PIN);

  /* De-initialize pins of GPIOC port */
  HAL_GPIO_DeInit(HAL_GPIOC, USER_LED_PIN_PIN | HAL_GPIO_PIN_1 | SPI2_CS1_PIN_PIN | OUT_5V_EN_PIN_PIN | HEAT_5V_EN_PIN_PIN);

  return SYSTEM_OK;
}

hal_exti_handle_t *mx_gpio_default_exti15_gethandle(void)
{
  return &hEXTI15;
}

/******************************************************************************/
/*                           EXTI Line15 interrupt                            */
/******************************************************************************/
void EXTI15_IRQHandler(void)
{
  HAL_EXTI_IRQHandler(&hEXTI15);
}
