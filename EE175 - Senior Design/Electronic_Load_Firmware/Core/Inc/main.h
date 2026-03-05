/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define VSENSE_Pin GPIO_PIN_1
#define VSENSE_GPIO_Port GPIOC
#define DEV_DET_Pin GPIO_PIN_2
#define DEV_DET_GPIO_Port GPIOC
#define V_SHUNT_1_Pin GPIO_PIN_0
#define V_SHUNT_1_GPIO_Port GPIOA
#define V_SHUNT_2_Pin GPIO_PIN_1
#define V_SHUNT_2_GPIO_Port GPIOA
#define V_SHUNT_3_Pin GPIO_PIN_2
#define V_SHUNT_3_GPIO_Port GPIOA
#define V_SHUNT_4_Pin GPIO_PIN_3
#define V_SHUNT_4_GPIO_Port GPIOA
#define ENC_A_Pin GPIO_PIN_5
#define ENC_A_GPIO_Port GPIOA
#define ENC_A_EXTI_IRQn EXTI9_5_IRQn
#define ENC_B_Pin GPIO_PIN_6
#define ENC_B_GPIO_Port GPIOA
#define ENC_B_EXTI_IRQn EXTI9_5_IRQn
#define ENC_SW_Pin GPIO_PIN_7
#define ENC_SW_GPIO_Port GPIOA
#define ENC_SW_EXTI_IRQn EXTI9_5_IRQn
#define OFF_Pin GPIO_PIN_0
#define OFF_GPIO_Port GPIOB
#define LOAD_3_Pin GPIO_PIN_1
#define LOAD_3_GPIO_Port GPIOB
#define LOAD_2_Pin GPIO_PIN_5
#define LOAD_2_GPIO_Port GPIOC
#define FAN_TACH_Pin GPIO_PIN_6
#define FAN_TACH_GPIO_Port GPIOC
#define SCL1_Pin GPIO_PIN_15
#define SCL1_GPIO_Port GPIOA
#define SWD_SWO_Pin GPIO_PIN_3
#define SWD_SWO_GPIO_Port GPIOB
#define SDA1_Pin GPIO_PIN_7
#define SDA1_GPIO_Port GPIOB
#define FAN_EN_Pin GPIO_PIN_9
#define FAN_EN_GPIO_Port GPIOB
#define LOAD_1_Pin GPIO_PIN_10
#define LOAD_1_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
