/**
  ******************************************************************************
  * File Name          : I2C.c
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

/* Includes ------------------------------------------------------------------*/
#include "i2c.h"

/* USER CODE BEGIN 0 */

/*
 * DS1307

     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f    0123456789abcdef
00: 33 15 00 01 01 01 00 00 03 33 2a 28 00 32 1c 88    3?.???..?3*(.2??
10: 00 1a 00 00 00 00 00 00 00 00 00 00 00 00 00 00    .?..............
20: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00    ................
30: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00    ................

 */


/* USER CODE END 0 */

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

/* I2C1 init function */
void MX_I2C1_Init(void)
{

  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00602173;
  hi2c1.Init.OwnAddress1 = 32;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Analogue filter 
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Digital filter 
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }

}
/* I2C2 init function */
void MX_I2C2_Init(void)
{

  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x10707DBC;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Analogue filter 
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Digital filter 
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }

}

void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(i2cHandle->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspInit 0 */

  /* USER CODE END I2C1_MspInit 0 */
  
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**I2C1 GPIO Configuration    
    PA9     ------> I2C1_SCL
    PA10     ------> I2C1_SDA 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF6_I2C1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* I2C1 clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();

    /* I2C1 interrupt Init */
    HAL_NVIC_SetPriority(I2C1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C1_IRQn);
  /* USER CODE BEGIN I2C1_MspInit 1 */

  /* USER CODE END I2C1_MspInit 1 */
  }
  else if(i2cHandle->Instance==I2C2)
  {
  /* USER CODE BEGIN I2C2_MspInit 0 */

  /* USER CODE END I2C2_MspInit 0 */
  
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C2 GPIO Configuration    
    PB10     ------> I2C2_SCL
    PB11     ------> I2C2_SDA 
    */
    GPIO_InitStruct.Pin = SCL2_Pin|GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF6_I2C2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* I2C2 clock enable */
    __HAL_RCC_I2C2_CLK_ENABLE();
  /* USER CODE BEGIN I2C2_MspInit 1 */

  /* USER CODE END I2C2_MspInit 1 */
  }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{

  if(i2cHandle->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspDeInit 0 */

  /* USER CODE END I2C1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C1_CLK_DISABLE();
  
    /**I2C1 GPIO Configuration    
    PA9     ------> I2C1_SCL
    PA10     ------> I2C1_SDA 
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

    /* I2C1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(I2C1_IRQn);
  /* USER CODE BEGIN I2C1_MspDeInit 1 */

  /* USER CODE END I2C1_MspDeInit 1 */
  }
  else if(i2cHandle->Instance==I2C2)
  {
  /* USER CODE BEGIN I2C2_MspDeInit 0 */

  /* USER CODE END I2C2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C2_CLK_DISABLE();
  
    /**I2C2 GPIO Configuration    
    PB10     ------> I2C2_SCL
    PB11     ------> I2C2_SDA 
    */
    HAL_GPIO_DeInit(GPIOB, SCL2_Pin|GPIO_PIN_11);

  /* USER CODE BEGIN I2C2_MspDeInit 1 */

  /* USER CODE END I2C2_MspDeInit 1 */
  }
} 

/* USER CODE BEGIN 1 */

enum { stop, waiting, getRegisterAddress, getData, sendData } transferState = stop;
uint8_t registerAddress = 0;

#define I2C_BUFFER_SIZE 1024
union {
	uint8_t u8[I2C_BUFFER_SIZE];
	uint16_t u16[I2C_BUFFER_SIZE/2];
	uint32_t u32[I2C_BUFFER_SIZE/4];
} i2c_buffer = {};

uint16_t receiveBuffer;

void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t direction, uint16_t addrMatchCode) {
	uint8_t transmitBuffer[2] = {};
	switch (direction) {
	case I2C_DIRECTION_TRANSMIT:
		transferState = getRegisterAddress;
		if (HAL_I2C_Slave_Sequential_Receive_IT(hi2c, &receiveBuffer, 1, I2C_FIRST_FRAME) != HAL_OK) {
			//Error();
		}
		break;

	case I2C_DIRECTION_RECEIVE:
		transferState = sendData;

		if( (registerAddress) < I2C_BUFFER_SIZE )
		{
			transmitBuffer[1] = API_I2C1_u8Get(2*registerAddress+0);
			transmitBuffer[0] = API_I2C1_u8Get(2*registerAddress+1);
		}
		else
		{
			transmitBuffer[0] = 0xff;
			transmitBuffer[1] = 0xff;
		}

		if (HAL_I2C_Slave_Sequential_Transmit_IT(hi2c, &transmitBuffer, 2, I2C_LAST_FRAME) != HAL_OK) {
			// Error here!!! (HAL_BUSY)
			//Error();
		}
		break;
	}
}



void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c) {
	switch (transferState) {
	case getRegisterAddress:
		registerAddress = receiveBuffer;
		transferState = getData;
		if (HAL_I2C_Slave_Sequential_Receive_IT(hi2c, &receiveBuffer, 2, I2C_FIRST_FRAME) != HAL_OK) {
			//Error();
		}
		break;

	case getData:
		//if (!registerWriteEvent(registerAddress, receiveBuffer)) {
		//				Error();
		//}

		if (HAL_I2C_Slave_Sequential_Receive_IT(hi2c, &receiveBuffer, 2, I2C_FIRST_FRAME) != HAL_OK) {
			//Error();
		} else if( (registerAddress>>1) < 64 ) {
			API_I2C1_u16Set(registerAddress,receiveBuffer);
			//i2c_buffer.u16[registerAddress] = receiveBuffer;
		}
		break;
	}
}



void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c) {
	HAL_I2C_EnableListen_IT(hi2c); // Restart
}



void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
	if (HAL_I2C_GetError(hi2c) != HAL_I2C_ERROR_AF) {
		//Error();
	}
}

uint8_t API_I2C1_u8Get(uint16_t addr)
{
	if( (I2C_BUFFER_SIZE) < addr ) return 0;
	return i2c_buffer.u8[addr];
}

uint16_t API_I2C1_u16Get(uint16_t addr)
{
	if( (I2C_BUFFER_SIZE/2) < addr ) return 0;
	return i2c_buffer.u16[addr];
}

uint32_t API_I2C1_u32Get(uint16_t addr)
{
	if( (I2C_BUFFER_SIZE/4) < addr ) return 0;
	return i2c_buffer.u32[addr];
}

void API_I2C1_u8Set(uint16_t addr, uint8_t data)
{
	if( (I2C_BUFFER_SIZE) < addr ) return 0;
	i2c_buffer.u8[addr] = data;
}

void API_I2C1_u16Set(uint16_t addr, uint16_t data)
{
	if( (I2C_BUFFER_SIZE/2) < addr ) return 0;
	i2c_buffer.u16[addr] = data;
}

void API_I2C1_u32Set(uint16_t addr, uint32_t data)
{
	if( (I2C_BUFFER_SIZE/4) < addr ) return 0;
	i2c_buffer.u32[addr] = data;
}

void API_I2C1_Init(void)
{
	transferState = waiting;
	memset(&i2c_buffer,0,sizeof(i2c_buffer));

	if(HAL_I2C_EnableListen_IT(&hi2c1) != HAL_OK)
	{
		Error_Handler();
	}
}


/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
