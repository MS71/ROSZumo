/**
  ******************************************************************************
  * File Name          : I2C.c
  * Description        : This file provides code for the configuration
  *                      of the I2C instances.
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

uint8_t HAL_I2C_ErrorFlag = 1;

/* USER CODE END 0 */

I2C_HandleTypeDef hi2c1;

/* I2C1 init function */
void MX_I2C1_Init(void)
{

  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00300B29;
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
  /** I2C Fast mode Plus enable
  */
  HAL_I2CEx_EnableFastModePlus(I2C_FASTMODEPLUS_I2C1);

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
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9);

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_10);

    /* I2C1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(I2C1_IRQn);
  /* USER CODE BEGIN I2C1_MspDeInit 1 */

  /* USER CODE END I2C1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

enum { stop, waiting, getRegisterAddress, getData, sendData } transferState = stop;
uint8_t registerAddress = 0;

union {
	uint8_t u8[I2C_BUFFER_SIZE];
	uint16_t u16[I2C_BUFFER_SIZE/2];
	uint32_t u32[I2C_BUFFER_SIZE/4];
} i2c_buffer = {};

uint8_t i2c_buffer_wrflag[I2C_BUFFER_SIZE/8] = {};

uint8_t receiveBuffer;
uint8_t transmitBuffer;

char i2c_terminal_buffer[I2C_TERMINAL_BUFFER_SIZE] = {0};
uint8_t i2c_terminal_buffer_wrflag = 0;

void HAL_I2C_MemWriteCB(uint8_t addr);

void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t direction, uint16_t addrMatchCode) {
	switch (direction) {
	case I2C_DIRECTION_TRANSMIT:
		transferState = getRegisterAddress;
		if (HAL_I2C_Slave_Sequential_Receive_IT(hi2c, &receiveBuffer, 1, I2C_FIRST_FRAME) != HAL_OK) {
			Error_Handler();
		}
		break;

	case I2C_DIRECTION_RECEIVE:
		transferState = sendData;

		if( (registerAddress) < I2C_BUFFER_SIZE )
		{
			transmitBuffer = API_I2C1_u8Get(registerAddress);
		}
		else
		{
			transmitBuffer = 0xff;
		}
		registerAddress++;

		if (HAL_I2C_Slave_Sequential_Transmit_IT(hi2c, &transmitBuffer, 1, I2C_NEXT_FRAME) != HAL_OK) {
			// Error here!!! (HAL_BUSY)
			Error_Handler();
		}
		break;

	default:
		Error_Handler();
		break;

	}
}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	switch (transferState) {
	case sendData:
		if( (registerAddress) < I2C_BUFFER_SIZE )
		{
			transmitBuffer = API_I2C1_u8Get(registerAddress);
		}
		else
		{
			transmitBuffer = 0xff;
		}
		registerAddress++;

		if (HAL_I2C_Slave_Sequential_Transmit_IT(hi2c, &transmitBuffer, 1, I2C_NEXT_FRAME) != HAL_OK) {
			// Error here!!! (HAL_BUSY)
			Error_Handler();
		}
		break;
	}
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c) {
	switch (transferState) {
	case getRegisterAddress:
		registerAddress = receiveBuffer;
		transferState = getData;
		if (HAL_I2C_Slave_Sequential_Receive_IT(hi2c, &receiveBuffer, 1, I2C_FIRST_FRAME) != HAL_OK) {
			Error_Handler();
		}
		break;

	case getData:
		if (HAL_I2C_Slave_Sequential_Receive_IT(hi2c, &receiveBuffer, 1, I2C_NEXT_FRAME) != HAL_OK) {
			Error_Handler();
		}
		else if(registerAddress == 0xff )
		{
			int i;
			for( i=0; i<(I2C_TERMINAL_BUFFER_SIZE-1); i++ )
			{
				i2c_terminal_buffer[i] = i2c_terminal_buffer[i+1];
			}
			i2c_terminal_buffer[I2C_TERMINAL_BUFFER_SIZE-1] = receiveBuffer;
			i2c_terminal_buffer_wrflag = 1;
		}
		else if( (registerAddress>>1) < I2C_BUFFER_SIZE ) {
			API_I2C1_u8Set(registerAddress++,receiveBuffer);
			i2c_buffer_wrflag[registerAddress>>3] |= (1<<(registerAddress&7));
			HAL_I2C_MemWriteCB(registerAddress-1);
		}
		break;
	}
}

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c) {
	HAL_I2C_EnableListen_IT(hi2c); // Restart
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
#if 0
	if (HAL_I2C_GetError(hi2c) != HAL_I2C_ERROR_BERR) {
		Error_Handler();
	}
	if (HAL_I2C_GetError(hi2c) != HAL_I2C_ERROR_ARLO) {
		Error_Handler();
	}
#endif
	if (HAL_I2C_GetError(hi2c) != HAL_I2C_ERROR_AF) {
		Error_Handler();
	}
#if 0
	if (HAL_I2C_GetError(hi2c) != HAL_I2C_ERROR_OVR) {
		Error_Handler();
	}
	if (HAL_I2C_GetError(hi2c) != HAL_I2C_ERROR_DMA) {
		Error_Handler();
	}
#endif
#if 0
	if (HAL_I2C_GetError(hi2c) != HAL_I2C_ERROR_TIMEOUT) {
		Error_Handler();
	}
#endif
}

uint8_t API_I2C1_u8WRFlag(uint16_t addr,uint8_t w)
{
	uint8_t ret = 0;
	if( addr < (I2C_BUFFER_SIZE) )
	{
		int i;
		for(i=0;i<w;i++)
		{
			ret |= ((i2c_buffer_wrflag[(addr+i)>>3]&(1<<((addr+i)&7)))!=0)?1:0;
			i2c_buffer_wrflag[(addr+i)>>3] &= ~(1<<((addr+i)&7));
		}
		return ret;
	} else if( addr >= I2C_REG_TB_U16_TERMINALBUFFER && addr < (I2C_REG_TB_U16_TERMINALBUFFER+I2C_TERMINAL_BUFFER_SIZE) )
	{
		ret = i2c_terminal_buffer_wrflag;
		i2c_terminal_buffer_wrflag = 0;
		return ret;
	}
	return 0;
}

uint8_t API_I2C1_u8Get(uint16_t addr)
{
	if( addr < (I2C_BUFFER_SIZE) )
	{
		return i2c_buffer.u8[addr];
	} else if( addr >= I2C_REG_TB_U16_TERMINALBUFFER && addr < (I2C_REG_TB_U16_TERMINALBUFFER+I2C_TERMINAL_BUFFER_SIZE) )
    {
		return i2c_terminal_buffer[addr-I2C_REG_TB_U16_TERMINALBUFFER];
	}
	return 0;
}

uint16_t API_I2C1_u16Get(uint16_t addr)
{
	if( (I2C_BUFFER_SIZE) < addr ) return 0;
	//return i2c_buffer.u16[addr>>1];
	return (i2c_buffer.u8[addr+1]<<8)|(i2c_buffer.u8[addr+0]<<0);
}

uint32_t API_I2C1_u32Get(uint16_t addr)
{
	if( (I2C_BUFFER_SIZE) < addr ) return 0;
	//return i2c_buffer.u32[addr>>2];
	return (i2c_buffer.u8[addr+3]<<24)|(i2c_buffer.u8[addr+2]<<16)|(i2c_buffer.u8[addr+1]<<8)|(i2c_buffer.u8[addr+0]<<0);
}

void API_I2C1_u8Set(uint16_t addr, uint8_t data)
{
	if( addr < (I2C_BUFFER_SIZE) )
	{
		i2c_buffer.u8[addr] = data;
		i2c_buffer_wrflag[(addr+0)>>3] |= (1<<((addr+0)&7));
	} else if( addr >= I2C_REG_TB_U16_TERMINALBUFFER && addr < (I2C_REG_TB_U16_TERMINALBUFFER+I2C_TERMINAL_BUFFER_SIZE) )
	{
		int i;
		for( i=0; i<(I2C_TERMINAL_BUFFER_SIZE-1); i++ )
		{
			i2c_terminal_buffer[i] = i2c_terminal_buffer[i+1];
		}
		i2c_terminal_buffer[I2C_TERMINAL_BUFFER_SIZE-1] = data;
		i2c_terminal_buffer_wrflag = 1;
	}
}

void API_I2C1_u16Set(uint16_t addr, uint16_t data)
{
	if( (I2C_BUFFER_SIZE) < addr ) return;
	//i2c_buffer.u16[addr>>1] = data;
	i2c_buffer.u8[addr+1] = (data>>8)&0xff;
	i2c_buffer.u8[addr+0] = (data>>0)&0xff;
	i2c_buffer_wrflag[(addr+0)>>3] |= (1<<((addr+0)&7));
	i2c_buffer_wrflag[(addr+1)>>3] |= (1<<((addr+1)&7));
}

void API_I2C1_u32Set(uint16_t addr, uint32_t data)
{
	if( (I2C_BUFFER_SIZE) < addr ) return;
	//i2c_buffer.u32[addr>>2] = data;
	i2c_buffer.u8[addr+3] = (data>>24)&0xff;
	i2c_buffer.u8[addr+2] = (data>>16)&0xff;
	i2c_buffer.u8[addr+1] = (data>>8)&0xff;
	i2c_buffer.u8[addr+0] = (data>>0)&0xff;
	i2c_buffer_wrflag[(addr+0)>>3] |= (1<<((addr+0)&7));
	i2c_buffer_wrflag[(addr+1)>>3] |= (1<<((addr+1)&7));

	i2c_buffer_wrflag[(addr+2)>>3] |= (1<<((addr+2)&7));
	i2c_buffer_wrflag[(addr+3)>>3] |= (1<<((addr+3)&7));
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

void API_I2C1_Handle(void)
{
}

void API_I2C1_Restart(void)
{
	HAL_I2C_StateTypeDef state = HAL_I2C_GetState(&hi2c1);
	if( state != HAL_I2C_STATE_LISTEN )
	{
		__HAL_I2C_RESET_HANDLE_STATE(&hi2c1);
	}
}


/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
