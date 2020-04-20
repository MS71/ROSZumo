/**
  ******************************************************************************
  * File Name          : I2C.h
  * Description        : This file provides code for the configuration
  *                      of the I2C instances.
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __i2c_H
#define __i2c_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN Private defines */

#define I2C_BUFFER_SIZE 128

#define I2C_REG_DS1307_REG0 				0x0000

#define I2C_REG_TB_U32_LOOP_CNT				0x0040
#define I2C_REG_TB_U8_PWRMODE				0x0044
#define PWRMODE_UNDEF	0x00
#define PWRMODE_OFF		0x01
#define PWRMODE_ON		0x02
#define PWRMODE_AUTO	0x03
#define PWRMODE_DEFAULT	PWRMODE_ON
#define I2C_REG_TB_U8_PWRCNTDWN				0x0045
#define PWRCNTDWN_START	  10
#define I2C_REG_TB_U8_PWRMODE_NEXT			0x0046
#define I2C_REG_TB_U8_PWR_TMPON  			0x0047
#define PWR_TMPON_TIME	  10

#define I2C_REG_TB_U16_UBAT_MV				0x0050
#define I2C_REG_TB_U16_UCHARGE_MV			0x0052
#define I2C_REG_TB_U16_TEMP_C				0x0054

#define I2C_REG_TB_U16_VL53L1X_RSTREG		0x0060

#define I2C_REG_TB_U16_TERMINALBUFFER		1000
#define I2C_TERMINAL_BUFFER_SIZE 		    1000

/* USER CODE END Private defines */

void MX_I2C1_Init(void);

/* USER CODE BEGIN Prototypes */

void API_I2C1_Init(void);

uint8_t  API_I2C1_u8Get(uint16_t addr);
uint16_t API_I2C1_u16Get(uint16_t addr);
uint32_t API_I2C1_u32Get(uint16_t addr);

void API_I2C1_u8Set(uint16_t addr, uint8_t data);
void API_I2C1_u16Set(uint16_t addr, uint16_t data);
void API_I2C1_u32Set(uint16_t addr, uint32_t data);

uint8_t API_I2C1_u8WRFlag(uint16_t addr);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ i2c_H */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
