/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#define T_SW2_Pin GPIO_PIN_6
#define T_SW2_GPIO_Port GPIOE
#define T_SW1_Pin GPIO_PIN_3
#define T_SW1_GPIO_Port GPIOC
#define D_SW4_Pin GPIO_PIN_5
#define D_SW4_GPIO_Port GPIOC
#define D_SW1_Pin GPIO_PIN_0
#define D_SW1_GPIO_Port GPIOB
#define D_SW3_Pin GPIO_PIN_1
#define D_SW3_GPIO_Port GPIOB
#define D_SW2_Pin GPIO_PIN_2
#define D_SW2_GPIO_Port GPIOB
#define IOG5_Pin GPIO_PIN_7
#define IOG5_GPIO_Port GPIOE
#define IOG4_Pin GPIO_PIN_8
#define IOG4_GPIO_Port GPIOE
#define IOG3_Pin GPIO_PIN_9
#define IOG3_GPIO_Port GPIOE
#define IOG2_Pin GPIO_PIN_10
#define IOG2_GPIO_Port GPIOE
#define IOG1_Pin GPIO_PIN_11
#define IOG1_GPIO_Port GPIOE
#define ALERT_Pin GPIO_PIN_12
#define ALERT_GPIO_Port GPIOE
#define T_SW8_Pin GPIO_PIN_14
#define T_SW8_GPIO_Port GPIOD
#define T_SW7_Pin GPIO_PIN_15
#define T_SW7_GPIO_Port GPIOD
#define T_SW6_Pin GPIO_PIN_11
#define T_SW6_GPIO_Port GPIOA
#define T_SW5_Pin GPIO_PIN_12
#define T_SW5_GPIO_Port GPIOA
#define T_SW4_Pin GPIO_PIN_0
#define T_SW4_GPIO_Port GPIOE
#define T_SW3_Pin GPIO_PIN_1
#define T_SW3_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */
#define MAX_UART_DATA_PACKET_SIZE 512 // максимальный размер пакета данных по UART (rx/tx)
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
