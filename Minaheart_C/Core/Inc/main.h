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
#include "stm32f4xx_hal.h"

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
void myprintf(const char *fmt, ...);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LS_1_Pin GPIO_PIN_14
#define LS_1_GPIO_Port GPIOC
#define LS_2_Pin GPIO_PIN_15
#define LS_2_GPIO_Port GPIOC
#define SERVO_1_Pin GPIO_PIN_0
#define SERVO_1_GPIO_Port GPIOA
#define SERVO_2_Pin GPIO_PIN_1
#define SERVO_2_GPIO_Port GPIOA
#define SERVO_3_Pin GPIO_PIN_2
#define SERVO_3_GPIO_Port GPIOA
#define SERVO_4_Pin GPIO_PIN_3
#define SERVO_4_GPIO_Port GPIOA
#define BMP580_CS_Pin GPIO_PIN_4
#define BMP580_CS_GPIO_Port GPIOA
#define BMP580_SCK_Pin GPIO_PIN_5
#define BMP580_SCK_GPIO_Port GPIOA
#define BMP580_MISO_Pin GPIO_PIN_6
#define BMP580_MISO_GPIO_Port GPIOA
#define BMP580_MOSI_Pin GPIO_PIN_7
#define BMP580_MOSI_GPIO_Port GPIOA
#define LoRa_RESET_Pin GPIO_PIN_0
#define LoRa_RESET_GPIO_Port GPIOB
#define LoRa_CS_Pin GPIO_PIN_1
#define LoRa_CS_GPIO_Port GPIOB
#define SD_CS_Pin GPIO_PIN_2
#define SD_CS_GPIO_Port GPIOB
#define BMI160_SCL_Pin GPIO_PIN_10
#define BMI160_SCL_GPIO_Port GPIOB
#define SD_SCK_Pin GPIO_PIN_13
#define SD_SCK_GPIO_Port GPIOB
#define SD_MISO_Pin GPIO_PIN_14
#define SD_MISO_GPIO_Port GPIOB
#define SD_MOSI_Pin GPIO_PIN_15
#define SD_MOSI_GPIO_Port GPIOB
#define LoRa_DIO0_Pin GPIO_PIN_8
#define LoRa_DIO0_GPIO_Port GPIOA
#define LS_3_Pin GPIO_PIN_11
#define LS_3_GPIO_Port GPIOA
#define LS_4_Pin GPIO_PIN_12
#define LS_4_GPIO_Port GPIOA
#define LoRa_SCK_Pin GPIO_PIN_3
#define LoRa_SCK_GPIO_Port GPIOB
#define LoRa_MISO_Pin GPIO_PIN_4
#define LoRa_MISO_GPIO_Port GPIOB
#define LoRa_MOSI_Pin GPIO_PIN_5
#define LoRa_MOSI_GPIO_Port GPIOB
#define BMI160_SDA_Pin GPIO_PIN_9
#define BMI160_SDA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define SD_SPI_HANDLE hspi2
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
