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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */

// I2C1 (DAC MCP4725)
// Schematic: SCL=PB8, SDA=PB9
#define DAC_I2C_SCL_Pin GPIO_PIN_8
#define DAC_I2C_SCL_Port GPIOB
#define DAC_I2C_SDA_Pin GPIO_PIN_9
#define DAC_I2C_SDA_Port GPIOB

// ADC (Internal)
// PC2 -> VSENSE (ADC1_IN12 on F4)
#define VSENSE_Pin GPIO_PIN_2
#define VSENSE_Port GPIOC

// V_SHUNT (Placeholder - User didn't specify exact pins 1-4, assumng PA0-3)
#define VSHUNT_1_Pin GPIO_PIN_0
#define VSHUNT_1_Port GPIOA

// Fan Control
// Schematic: FAN_PWM = PB6 (TIM4_CH1)
// Schematic: FAN_TACH = PC6 (Input)
#define FAN_PWM_Pin GPIO_PIN_6
#define FAN_PWM_Port GPIOB
#define FAN_TACH_Pin GPIO_PIN_6
#define FAN_TACH_Port GPIOC

// Safety / Gate Drive
// Schematic: OFF/KILL = PB0. 
// Logic: HIGH = OFF (Kill), LOW = ON (Run).
// Renamed to match sensors.c checks!
#define GATE_DISABLE_Pin GPIO_PIN_0 
#define GATE_DISABLE_Port GPIOB

// Rotary Encoder
// Schematic: A=PA5, B=PA6, SW=PB7
#define ENC_A_Pin GPIO_PIN_5
#define ENC_A_Port GPIOA
#define ENC_B_Pin GPIO_PIN_6
#define ENC_B_Port GPIOA
#define ENC_SW_Pin GPIO_PIN_7
#define ENC_SW_Port GPIOB

// USB / Debug (Standard)
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_Port GPIOA

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
