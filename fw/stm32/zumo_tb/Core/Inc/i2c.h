/**
  ******************************************************************************
  * @file    i2c.h
  * @brief   This file contains all the function prototypes for
  *          the i2c.c file
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
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
#ifndef __I2C_H__
#define __I2C_H__

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
#define I2C_REG_TB_U8_PWRMODE_NEXT			0x0046
#define I2C_REG_TB_U8_PWR_TMPON  			0x0047
#define I2C_REG_TB_U8_LDS_SPEED				0x0048 /* N * 60 RPM */
#define I2C_REG_TB_U8_WIFIFST				0x0049
#define I2C_REG_TB_U32_IPADDR				0x004a
#define I2C_REG_TB_U16_VL53L1X_RSTREG		0x004C
#define I2C_REG_TB_U8_INT_ENABLE			0x004E
#define I2C_REG_TB_U8_INT_FLAGS				0x004F
#define I2C_REG_TB_U8_INT_BIT__LDS_READY	(1<<0)

#define I2C_REG_TB_U16_UBAT_MV				0x0050
#define I2C_REG_TB_U16_UCHARGE_MV			0x0052
#define I2C_REG_TB_U16_TEMP_C				0x0054
#define I2C_REG_TB_U16_UBAT_IDLE_MV			0x0056
#define I2C_REG_TB_U16_UBAT_CHARGE_MV		0x0058
#define I2C_REG_TB_U16_UBAT_MEAN_MV			0x005a
#define I2C_REG_TB_S16_UBAT_DIR				0x005c
#define I2C_REG_TB_U16_UBAT_LEVEL			0x005e

#define I2C_REG_TB_U16_TON_TOUT				0x0060
#define I2C_REG_TB_U16_TON_PERIOD			0x0062
#define I2C_REG_TB_U16_TON_WDG 				0x0064
#define I2C_REG_TB_U16_TOFF_TOUT			0x0066
#define I2C_REG_TB_U16_TOFF_PERIOD			0x0068

#define I2C_REG_TB_U16_RPI_DIR				0x006A
#define I2C_REG_TB_U16_RPI_READ				0x006C
#define I2C_REG_TB_U16_RPI_WRITE			0x006E

#define I2C_REG_TB_STREAM_REG_START			0x0070

#define I2C_REG_TB_U8_LDS_STREAM			0x0070
#define I2C_REG_TB_U8_TERMINAL_STREAM		0x0071
#define I2C_TERMINAL_BUFFER_SIZE 		    1000

/* USER CODE END Private defines */

void MX_I2C1_Init(void);

/* USER CODE BEGIN Prototypes */

void API_I2C1_Init(void);
void API_I2C1_Restart(void);
void API_I2C1_Handle(void);

uint8_t  API_I2C1_u8Get(uint16_t addr);
uint16_t API_I2C1_u16Get(uint16_t addr);
uint32_t API_I2C1_u32Get(uint16_t addr);

void API_I2C1_u8Set(uint16_t addr, uint8_t data);
void API_I2C1_u16Set(uint16_t addr, uint16_t data);
void API_I2C1_u32Set(uint16_t addr, uint32_t data);

uint8_t API_I2C1_u8WRFlag(uint16_t addr,uint8_t w);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __I2C_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
