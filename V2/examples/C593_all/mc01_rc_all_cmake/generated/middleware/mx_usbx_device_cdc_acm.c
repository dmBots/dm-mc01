/**
  ******************************************************************************
  * @file    mx_usbx_device_cdc_acm.c
  * @brief   USBX Device CDC ACM applicative source file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025-2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the mx_usbx_license.md file
  * in the same directory as the generated code.
  * If no mx_usbx_license.md file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "mx_usbx_device_cdc_acm.h"
#include "bsp_usb_cdc.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/**
  * @brief  usbd_cdc_acm_activate
  *         This function is called when insertion of a CDC ACM device.
  * @param  cdc_acm_instance: Pointer to the cdc acm class instance.
  * @retval none
  */
VOID usbd_cdc_acm_activate(VOID *cdc_acm_instance)
{
  bsp_usb_cdc_on_activate(cdc_acm_instance);

  return;
}

/**
  * @brief  usbd_cdc_acm_deactivate
  *         This function is called when extraction of a CDC ACM device.
  * @param  cdc_acm_instance: Pointer to the cdc acm class instance.
  * @retval none
  */
VOID usbd_cdc_acm_deactivate(VOID *cdc_acm_instance)
{
  bsp_usb_cdc_on_deactivate(cdc_acm_instance);

  return;
}

/**
  * @brief  usbd_cdc_acm_ParameterChange
  *         This function is invoked to manage the CDC ACM class requests.
  * @param  cdc_acm_instance: Pointer to the cdc acm class instance.
  * @retval none
  */
VOID usbd_cdc_acm_ParameterChange(VOID *cdc_acm_instance)
{
  bsp_usb_cdc_on_parameter_change(cdc_acm_instance);

  return;
}
