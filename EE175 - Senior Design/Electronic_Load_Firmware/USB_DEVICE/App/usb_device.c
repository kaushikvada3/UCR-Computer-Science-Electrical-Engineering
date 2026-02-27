/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usb_device.c
  * @version        : v2.0_Cube
  * @brief          : This file implements the USB Device
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/

#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USB Device Core handle declaration. */
USBD_HandleTypeDef hUsbDeviceFS;

/*
 * -- Insert your variables declaration here --
 */
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*
 * -- Insert your external function declaration here --
 */
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/**
  * Init USB device Library, add supported class and start the library
  * @retval None
  */
void MX_USB_DEVICE_Init(void)
{
  /* USER CODE BEGIN USB_DEVICE_Init_PreTreatment */

  /* --- Workaround: HAL USB_DevInit() missing tSTARTUP delay ---
   * RM0316 Section 32.4.2 requires ~1 us between clearing PDWN and
   * clearing FRES so the analog transceiver stabilises.  The HAL
   * does both back-to-back.  We pre-sequence the power-up here,
   * BEFORE the USB stack configures endpoints, so nothing is wiped.
   */
  {
    __HAL_RCC_USB_CLK_ENABLE();

    /* Ensure D+ pull-up is off so host sees no device yet */
    {
      GPIO_InitTypeDef g = {0};
      __HAL_RCC_GPIOC_CLK_ENABLE();
      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
      g.Pin   = GPIO_PIN_2;
      g.Mode  = GPIO_MODE_OUTPUT_PP;
      g.Pull  = GPIO_NOPULL;
      g.Speed = GPIO_SPEED_FREQ_LOW;
      HAL_GPIO_Init(GPIOC, &g);
    }

    /* Power-down + force-reset */
    USB->CNTR = (uint16_t)(USB_CNTR_FRES | USB_CNTR_PDWN);
    HAL_Delay(1);

    /* Clear PDWN, keep FRES — analog transceiver powers up */
    USB->CNTR = (uint16_t)USB_CNTR_FRES;
    HAL_Delay(1);   /* tSTARTUP */

    /* Release reset — peripheral is now ready for HAL_PCD_Init */
    USB->CNTR = 0U;
    USB->ISTR = 0U;

    __HAL_RCC_USB_CLK_DISABLE();
  }

  /* USER CODE END USB_DEVICE_Init_PreTreatment */

  /* Init Device Library, add supported class and start the library. */
  if (USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS) != USBD_OK)
  {
    Error_Handler();
  }
  if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC) != USBD_OK)
  {
    Error_Handler();
  }
  if (USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS) != USBD_OK)
  {
    Error_Handler();
  }
  if (USBD_Start(&hUsbDeviceFS) != USBD_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN USB_DEVICE_Init_PostTreatment */

  /* Let the host de-bounce, then assert D+ pull-up for a clean attach.
   * USBD_Start() already called HAL_PCD_Start which enables interrupts
   * and sets up endpoints/PMA — we must NOT touch USB registers here. */
  HAL_Delay(50);
  {
    PCD_HandleTypeDef *hpcd = (PCD_HandleTypeDef *)hUsbDeviceFS.pData;
    HAL_PCDEx_SetConnectionState(hpcd, 1U);
  }

  /* USER CODE END USB_DEVICE_Init_PostTreatment */
}

/**
  * @}
  */

/**
  * @}
  */

