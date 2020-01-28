/*
 * pm.c
 *
 *  Created on: Jan 16, 2020
 *      Author: maik
 */
#include <stdio.h>

#include "main.h"
#include "adc.h"
#include "aes.h"
#include "crc.h"
#include "i2c.h"
#include "lptim.h"
#include "rng.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

#include "pm.h"

#define ADC_CONVERTED_DATA_BUFFER_SIZE 5
__IO   uint16_t   aADCxConvertedData[ADC_CONVERTED_DATA_BUFFER_SIZE]; /* ADC group regular conversion data (array of data) */

uint8_t ubDmaTransferStatus = 0;
/**
  * @brief  Conversion complete callback in non blocking mode
  * @param  hadc: ADC handle
  * @note   This example shows a simple way to report end of conversion
  *         and get conversion result. You can add your own implementation.
  * @retval None
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  /* Update status variable of DMA transfer */
  ubDmaTransferStatus = 1;

  /* Set LED depending on DMA transfer status */
  /* - Turn-on if DMA transfer is completed */
  /* - Turn-off if DMA transfer is not completed */
  //BSP_LED_On(LED1);
}

/**
  * @brief  Conversion DMA half-transfer callback in non blocking mode
  * @note   This example shows a simple way to report end of conversion
  *         and get conversion result. You can add your own implementation.
  * @retval None
  */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
  /* Set LED depending on DMA transfer status */
  /* - Turn-on if DMA transfer is completed */
  /* - Turn-off if DMA transfer is not completed */
  //BSP_LED_Off(LED1);
}

/**
  * @brief  ADC error callback in non blocking mode
  *        (ADC conversion with interruption or transfer by DMA)
  * @param  hadc: ADC handle
  * @retval None
  */
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
  /* In case of ADC error, call main error handler */
  Error_Handler();
}

void pm_init()
{
	ubDmaTransferStatus = 0;

	if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
	{
		/* Calibration Error */
		Error_Handler();
	}

	if (HAL_ADC_Start_DMA(&hadc1,
			(uint32_t *)aADCxConvertedData,
			ADC_CONVERTED_DATA_BUFFER_SIZE
	) != HAL_OK)
	{
		/* ADC conversion start error */
		Error_Handler();
	}

	API_I2C1_u8Set(I2C_REG_TB_U8_PWRCNTDWN,0);
	API_I2C1_u8Set(I2C_REG_TB_U8_PWRMODE,PWRMODE_DEFAULT);
	API_I2C1_u8Set(I2C_REG_TB_U8_PWRMODE_NEXT,PWRMODE_UNDEF);

}

void pm_exit()
{

}

void pm_loop()
{
	{
		static uint32_t t_ = 0;
		uint32_t t = HAL_GetTick();
		if( t > t_ )
		{
			t_ = t + 1000;

			if( API_I2C1_u8Get(I2C_REG_TB_U8_PWRMODE_NEXT) != PWRMODE_UNDEF )
			{
				if( API_I2C1_u8Get(I2C_REG_TB_U8_PWRCNTDWN) != 0 )
				{
					API_I2C1_u8Set(I2C_REG_TB_U8_PWRCNTDWN,API_I2C1_u8Get(I2C_REG_TB_U8_PWRCNTDWN)-1);
					if( API_I2C1_u8Get(I2C_REG_TB_U8_PWRCNTDWN) == 0 )
					{
						API_I2C1_u8Set(I2C_REG_TB_U8_PWRMODE,API_I2C1_u8Get(I2C_REG_TB_U8_PWRMODE_NEXT));
						API_I2C1_u8Set(I2C_REG_TB_U8_PWRMODE_NEXT,PWRMODE_UNDEF);
					}
				}
			}
		}
	}

    /* Start ADC conversion */
    /* Since sequencer is enabled in discontinuous mode, this will perform    */
    /* the conversion of the next rank in sequencer.                          */
    /* Note: For this example, conversion is triggered by software start,     */
    /*       therefore "HAL_ADC_Start()" must be called for each conversion.  */
    /*       Since DMA transfer has been initiated previously by function     */
    /*       "HAL_ADC_Start_DMA()", this function will keep DMA transfer      */
    /*       active.                                                          */

    if (HAL_ADC_Start(&hadc1) != HAL_OK)
    {
      Error_Handler();
    }

    /* Wait for ADC conversion and DMA transfer completion (update of variable ubDmaTransferStatus) */
    HAL_Delay(1);

    /* Check whether ADC has converted all ranks of the sequence */
    if (ubDmaTransferStatus == 1)
    {
    	ubDmaTransferStatus = 0;

    	int v;
    	uint16_t vdda = __LL_ADC_CALC_VREFANALOG_VOLTAGE( aADCxConvertedData[3], LL_ADC_RESOLUTION_12B);

    	int Rx = 21;
    	v = ((100+Rx)*(uint32_t)__LL_ADC_CALC_DATA_TO_VOLTAGE(vdda, aADCxConvertedData[1], LL_ADC_RESOLUTION_12B)/Rx);
    	API_I2C1_u16Set(I2C_REG_TB_U16_UBAT_MV,v);

    	v = (int)((100+Rx)*(uint32_t)__LL_ADC_CALC_DATA_TO_VOLTAGE(vdda, aADCxConvertedData[0], LL_ADC_RESOLUTION_12B)/Rx);
    	API_I2C1_u16Set(I2C_REG_TB_U16_UCHARGE_MV,v);

    	v = (int)__LL_ADC_CALC_TEMPERATURE(vdda, aADCxConvertedData[2], LL_ADC_RESOLUTION_12B);
    	API_I2C1_u16Set(I2C_REG_TB_U16_TEMP_C,v);
    }

	if( API_I2C1_u8Get(I2C_REG_TB_U8_PWRMODE) == PWRMODE_OFF )
	{
		HAL_GPIO_WritePin(O_LED2_GPIO_Port,O_LED2_Pin,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(ZUMO_SHDN_GPIO_Port,ZUMO_SHDN_Pin,GPIO_PIN_SET);

		HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

		SystemClock_Config();

		HAL_GPIO_WritePin(ZUMO_SHDN_GPIO_Port,ZUMO_SHDN_Pin,GPIO_PIN_RESET);
		ui_init();
	}
}
