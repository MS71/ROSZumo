/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
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
#include "stm32g0xx_hal.h"

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
#define I2C_ADDR 0x10
#define RPI_14_Pin GPIO_PIN_11
#define RPI_14_GPIO_Port GPIOC
#define RPI_15_Pin GPIO_PIN_12
#define RPI_15_GPIO_Port GPIOC
#define RPI_16_Pin GPIO_PIN_13
#define RPI_16_GPIO_Port GPIOC
#define RPI_18_Pin GPIO_PIN_14
#define RPI_18_GPIO_Port GPIOC
#define O_INT_Pin GPIO_PIN_15
#define O_INT_GPIO_Port GPIOC
#define O_CAM_PWDN_Pin GPIO_PIN_0
#define O_CAM_PWDN_GPIO_Port GPIOC
#define O_LED1_Pin GPIO_PIN_1
#define O_LED1_GPIO_Port GPIOC
#define ADC0_Pin GPIO_PIN_0
#define ADC0_GPIO_Port GPIOA
#define ADC1_Pin GPIO_PIN_1
#define ADC1_GPIO_Port GPIOA
#define O_LED2_Pin GPIO_PIN_2
#define O_LED2_GPIO_Port GPIOA
#define O_L_RST_10_Pin GPIO_PIN_4
#define O_L_RST_10_GPIO_Port GPIOA
#define SPI_CLK_Pin GPIO_PIN_5
#define SPI_CLK_GPIO_Port GPIOA
#define RPI_CS0_Pin GPIO_PIN_6
#define RPI_CS0_GPIO_Port GPIOA
#define RPI_CS1_Pin GPIO_PIN_7
#define RPI_CS1_GPIO_Port GPIOA
#define U1_ESP_RXD_Pin GPIO_PIN_4
#define U1_ESP_RXD_GPIO_Port GPIOC
#define U1_ESP_TXD_Pin GPIO_PIN_5
#define U1_ESP_TXD_GPIO_Port GPIOC
#define U3_ZUMO_TXD_Pin GPIO_PIN_0
#define U3_ZUMO_TXD_GPIO_Port GPIOB
#define O_L_RST_1_Pin GPIO_PIN_1
#define O_L_RST_1_GPIO_Port GPIOB
#define U3_ZUMO_RXD_Pin GPIO_PIN_2
#define U3_ZUMO_RXD_GPIO_Port GPIOB
#define SCL2_Pin GPIO_PIN_10
#define SCL2_GPIO_Port GPIOB
#define O_L_RST_10B12_Pin GPIO_PIN_12
#define O_L_RST_10B12_GPIO_Port GPIOB
#define O_L_RST_11_Pin GPIO_PIN_13
#define O_L_RST_11_GPIO_Port GPIOB
#define I_BUS_FON_Pin GPIO_PIN_14
#define I_BUS_FON_GPIO_Port GPIOB
#define ZUMO_SHDN_Pin GPIO_PIN_15
#define ZUMO_SHDN_GPIO_Port GPIOB
#define O_CHARGE_ON_Pin GPIO_PIN_8
#define O_CHARGE_ON_GPIO_Port GPIOA
#define O_L_RST_9_Pin GPIO_PIN_6
#define O_L_RST_9_GPIO_Port GPIOC
#define O_L_RST_8_Pin GPIO_PIN_7
#define O_L_RST_8_GPIO_Port GPIOC
#define O_LEDSTRB_Pin GPIO_PIN_8
#define O_LEDSTRB_GPIO_Port GPIOD
#define O_FLASH_CS_Pin GPIO_PIN_9
#define O_FLASH_CS_GPIO_Port GPIOD
#define SPI_MISO_Pin GPIO_PIN_11
#define SPI_MISO_GPIO_Port GPIOA
#define SPI_MOSI_Pin GPIO_PIN_12
#define SPI_MOSI_GPIO_Port GPIOA
#define SPI_CS_Pin GPIO_PIN_15
#define SPI_CS_GPIO_Port GPIOA
#define RPI_11_Pin GPIO_PIN_8
#define RPI_11_GPIO_Port GPIOC
#define RPI_12_Pin GPIO_PIN_9
#define RPI_12_GPIO_Port GPIOC
#define O_LIDAR_M_Pin GPIO_PIN_0
#define O_LIDAR_M_GPIO_Port GPIOD
#define O_BUS_RESET_Pin GPIO_PIN_1
#define O_BUS_RESET_GPIO_Port GPIOD
#define O_BNO055_RESET_Pin GPIO_PIN_2
#define O_BNO055_RESET_GPIO_Port GPIOD
#define O_L_RST_0_Pin GPIO_PIN_3
#define O_L_RST_0_GPIO_Port GPIOD
#define O_L_RST_2_Pin GPIO_PIN_4
#define O_L_RST_2_GPIO_Port GPIOD
#define O_FLASH_HOLD_Pin GPIO_PIN_5
#define O_FLASH_HOLD_GPIO_Port GPIOD
#define O_FLASH_WP_Pin GPIO_PIN_6
#define O_FLASH_WP_GPIO_Port GPIOD
#define O_L_RST_3_Pin GPIO_PIN_3
#define O_L_RST_3_GPIO_Port GPIOB
#define O_L_RST_4_Pin GPIO_PIN_4
#define O_L_RST_4_GPIO_Port GPIOB
#define O_L_RST_5_Pin GPIO_PIN_5
#define O_L_RST_5_GPIO_Port GPIOB
#define O_L_RST_6_Pin GPIO_PIN_6
#define O_L_RST_6_GPIO_Port GPIOB
#define O_L_RST_7_Pin GPIO_PIN_7
#define O_L_RST_7_GPIO_Port GPIOB
#define RPI_13_Pin GPIO_PIN_10
#define RPI_13_GPIO_Port GPIOC
/* USER CODE BEGIN Private defines */

#define LCD_CS_Port								RPI_CS0_GPIO_Port
#define LCD_CS_PIN								RPI_CS0_Pin

#define LCD_DC_Port								RPI_15_GPIO_Port
#define LCD_DC_PIN								RPI_15_Pin

#define	LCD_RST_Port							RPI_13_GPIO_Port
#define	LCD_RST_PIN								RPI_13_Pin

#define	KEY0_GPIO_Port							RPI_12_GPIO_Port
#define	KEY0_Pin								RPI_12_Pin

#define	KEY1_GPIO_Port							RPI_16_GPIO_Port
#define	KEY1_Pin								RPI_16_Pin

#define	KEY2_GPIO_Port							RPI_18_GPIO_Port
#define	KEY2_Pin								RPI_18_Pin

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
