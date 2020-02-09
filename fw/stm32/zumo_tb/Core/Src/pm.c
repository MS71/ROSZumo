/*
 * pm.c
 *
 *  Created on: Jan 16, 2020
 *      Author: maik
 */
#include <stdio.h>

#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

#include "pm.h"

void vStandby()
{
	if( API_I2C1_u8Get(I2C_REG_TB_U8_PWRMODE_NEXT) != PWRMODE_UNDEF )
	{
		HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0,0x42affe00|API_I2C1_u8Get(I2C_REG_TB_U8_PWRMODE_NEXT));
	}
	else
	{
		HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0,0x42affe00|API_I2C1_u8Get(I2C_REG_TB_U8_PWRMODE));
	}

	/* Clear Standby flag */
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);

	HAL_PWREx_EnableGPIOPullUp(PWR_GPIO_B/*ZUMO_SHDN_GPIO_Port*/,PWR_GPIO_BIT_15/*ZUMO_SHDN_Pin*/);
	HAL_PWREx_EnablePullUpPullDownConfig();

	//HAL_PWREx_EnableSRAMRetention();
	/* Clear all related wakeup flags*/
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF2);
	/* Enter the Standby mode */
	HAL_PWR_EnterSTANDBYMode();

	while(1);
}

#define ADC_CONVERTED_DATA_BUFFER_SIZE 5
__IO   uint16_t   aADCxConvertedData[ADC_CONVERTED_DATA_BUFFER_SIZE]; /* ADC group regular conversion data (array of data) */

#if 0
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
#endif

void pm_run_adc()
{
	int i;
	int n = 10;
	uint32_t sum_vdda = 0;
	uint32_t sum_ucharge = 0;
	uint32_t sum_ubat = 0;
	uint32_t sum_temp = 0;

	for(i=0;i<n;i++)
	{
		if (HAL_ADC_Start(&hadc1) != HAL_OK)
		{
			Error_Handler();
		}
		else if( HAL_ADC_PollForConversion(&hadc1,100) == HAL_OK )
		{
			sum_vdda += aADCxConvertedData[3];
			sum_ubat += aADCxConvertedData[1];
			sum_ucharge += aADCxConvertedData[0];
			sum_temp += aADCxConvertedData[2];

			HAL_ADC_Stop(&hadc1);
		}
	}

	{
		int v;
		uint16_t vdda = __LL_ADC_CALC_VREFANALOG_VOLTAGE( sum_vdda/n, LL_ADC_RESOLUTION_12B);

		int Rx = 21;
		v = ((100+Rx)*(uint32_t)__LL_ADC_CALC_DATA_TO_VOLTAGE(vdda, sum_ubat/n, LL_ADC_RESOLUTION_12B)/Rx);
		v = v * 10 / 12;
		API_I2C1_u16Set(I2C_REG_TB_U16_UBAT_MV,v);

		v = (int)((100+Rx)*(uint32_t)__LL_ADC_CALC_DATA_TO_VOLTAGE(vdda, sum_ucharge/n, LL_ADC_RESOLUTION_12B)/Rx);
		v = v * 10 / 12;
		API_I2C1_u16Set(I2C_REG_TB_U16_UCHARGE_MV,v);

		v = (int)__LL_ADC_CALC_TEMPERATURE(vdda, sum_temp/n, LL_ADC_RESOLUTION_12B);
		API_I2C1_u16Set(I2C_REG_TB_U16_TEMP_C,v);
	}
}

void pm_charge()
{
	uint8_t charging = 1;

	HAL_GPIO_WritePin(ZUMO_SHDN_GPIO_Port,ZUMO_SHDN_Pin,GPIO_PIN_SET);

	MX_DMA_Init();
	MX_ADC1_Init();

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

	/* Reduce the System clock to below 2 MHz */
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};

	/* Select HSI as system clock source */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
	if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
	{
		/* Initialization Error */
		Error_Handler();
	}

	/* Modify HSI to HSI DIV8 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV128;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		/* Initialization Error */
		Error_Handler();
	}

	HAL_GPIO_WritePin(O_LED2_GPIO_Port,O_LED2_Pin,GPIO_PIN_SET);
	pm_run_adc();
	HAL_GPIO_WritePin(O_LED2_GPIO_Port,O_LED2_Pin,GPIO_PIN_RESET);

	uint16_t ubat_pref = 0;
	uint16_t batfullcnt = 0;

	/* Set regulator voltage to scale 2 */
	HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2);
	while(batfullcnt < 10)
	{
		HAL_Delay(500);
		pm_run_adc();

		uint16_t u_bat = API_I2C1_u16Get(I2C_REG_TB_U16_UBAT_MV);
		uint16_t u_charge = API_I2C1_u16Get(I2C_REG_TB_U16_UCHARGE_MV);

		if( ubat_pref != 0 )
		{
			if( u_bat > ubat_pref )
			{
				batfullcnt=0;
			}
			else
			{
				batfullcnt++;
			}
		}
		ubat_pref = u_bat;

		if( UBAT_OK && UCHARGE_OK )
		{
			int c=9; /* 4500ms */
			HAL_GPIO_WritePin(O_CHARGE_ON_GPIO_Port,O_CHARGE_ON_Pin,GPIO_PIN_SET);
			HAL_GPIO_WritePin(O_LED2_GPIO_Port,O_LED2_Pin,GPIO_PIN_SET);
			while( c-- && UBAT_OK && UCHARGE_OK )
			{
				HAL_Delay(500);
				pm_run_adc();
			}
			HAL_GPIO_WritePin(O_LED2_GPIO_Port,O_LED2_Pin,GPIO_PIN_RESET);
			HAL_GPIO_WritePin(O_CHARGE_ON_GPIO_Port,O_CHARGE_ON_Pin,GPIO_PIN_RESET);
		}
		else
		{
			charging=0;
		}
	}
	/* Disable low power run mode and reset the clock to initialization configuration */
	HAL_PWREx_DisableLowPowerRunMode();
}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
	if( KEY2_Pin == GPIO_Pin )
	{
		uint8_t m = API_I2C1_u8Get(I2C_REG_TB_U8_PWRMODE_NEXT);
		if( m == PWRMODE_UNDEF )
		{
			m = API_I2C1_u8Get(I2C_REG_TB_U8_PWRMODE);
		}
		switch( m )
		{
		default:
		case PWRMODE_OFF:
			API_I2C1_u8Set(I2C_REG_TB_U8_PWRMODE_NEXT,PWRMODE_ON);
			API_I2C1_u8Set(I2C_REG_TB_U8_PWRCNTDWN,PWRCNTDWN_START);
			break;
		case PWRMODE_ON:
			API_I2C1_u8Set(I2C_REG_TB_U8_PWRMODE_NEXT,PWRMODE_AUTO);
			API_I2C1_u8Set(I2C_REG_TB_U8_PWRCNTDWN,PWRCNTDWN_START);
			break;
		case PWRMODE_AUTO:
			API_I2C1_u8Set(I2C_REG_TB_U8_PWRMODE_NEXT,PWRMODE_OFF);
			API_I2C1_u8Set(I2C_REG_TB_U8_PWRCNTDWN,PWRCNTDWN_START);
			break;
		}
		API_I2C1_u8Set(I2C_REG_TB_U8_PWR_TMPON,PWR_TMPON_TIME);
		HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0,0x42affe00|API_I2C1_u8Get(I2C_REG_TB_U8_PWRMODE_NEXT));
	}

	if( KEY0_Pin == GPIO_Pin )
	{
		vStandby();
	}
}

void pm_early_init()
{
	__HAL_RCC_PWR_CLK_ENABLE();
	SystemClock_Config();
	MX_RTC_Init();
	MX_GPIO_Init();

	HAL_GPIO_WritePin(O_LED2_GPIO_Port,O_LED2_Pin,GPIO_PIN_SET);

	uint32_t dr0 = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0);
	if( ((dr0&0xffffff00) == 0x42affe00 )&&(__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET))
	{
		API_I2C1_u8Set(I2C_REG_TB_U8_PWRMODE,dr0&0xff);
		if( (API_I2C1_u8Get(I2C_REG_TB_U8_PWRMODE)==PWRMODE_OFF) ||
				(API_I2C1_u8Get(I2C_REG_TB_U8_PWRMODE)==PWRMODE_AUTO) )
		{
			if( HAL_GPIO_ReadPin(KEY2_GPIO_Port,KEY2_Pin) != GPIO_PIN_RESET )
			{
				pm_charge();

				/* sleep again if not pressed */
				vStandby();
			}
		}
	}
	else
	{
		HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0,0x42affe00|PWRMODE_DEFAULT);
	}

	/* Clear Standby flag */
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);

}

void pm_init()
{
	//ubDmaTransferStatus = 0;

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
	API_I2C1_u8Set(I2C_REG_TB_U8_PWRMODE_NEXT,PWRMODE_UNDEF);
	API_I2C1_u8Set(I2C_REG_TB_U8_PWR_TMPON,PWR_TMPON_TIME);
    API_I2C1_u8Set(I2C_REG_TB_U8_PWRMODE,HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0)&0xff);

	//__HAL_RCC_PWR_CLK_ENABLE();
}

void pm_exit()
{

}

void pm_loop_internal()
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

			if( API_I2C1_u8Get(I2C_REG_TB_U8_PWR_TMPON) != 0 )
			{
				API_I2C1_u8Set(I2C_REG_TB_U8_PWR_TMPON,API_I2C1_u8Get(I2C_REG_TB_U8_PWR_TMPON)-1);
			}
		}
	}

#if 1
	{
		static uint32_t t_ = 0;
		uint32_t t = HAL_GetTick();
		if( t > t_ )
		{
			t_ = t + 5000;
		    HAL_GPIO_TogglePin(O_CHARGE_ON_GPIO_Port,O_CHARGE_ON_Pin);
		}
	}
#endif

#if 1
	pm_run_adc();

	{
		uint16_t u_bat = API_I2C1_u16Get(I2C_REG_TB_U16_UBAT_MV);
		if( !UBAT_OK )
		{
			/*
			 * auto shutdown when ubat is to low
			 */
			API_I2C1_u8Set(I2C_REG_TB_U8_PWRMODE_NEXT,PWRMODE_ON);
			API_I2C1_u8Set(I2C_REG_TB_U8_PWRCNTDWN,PWRCNTDWN_START);
		}
	}
#endif

#if 0
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
    //HAL_Delay(1);

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
#endif
}

void pm_loop()
{
	pm_loop_internal();

	//API_I2C1_u8Set(I2C_REG_TB_U8_PWRMODE,PWRMODE_OFF);

	if( API_I2C1_u8Get(I2C_REG_TB_U8_PWRMODE)==PWRMODE_OFF &&
			API_I2C1_u8Get(I2C_REG_TB_U8_PWR_TMPON)==0 )
	{
		HAL_GPIO_WritePin(ZUMO_SHDN_GPIO_Port,ZUMO_SHDN_Pin,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC, RST_Pin, GPIO_PIN_RESET);
		//HAL_SPI_DeInit(&hspi1);

#if 1
		API_I2C1_u8Set(I2C_REG_TB_U8_PWRMODE,0x42affe00|API_I2C1_u8Get(I2C_REG_TB_U8_PWRMODE));
		vStandby();
#else
		while( API_I2C1_u8Get(I2C_REG_TB_U8_PWRMODE)==PWRMODE_OFF &&
				API_I2C1_u8Get(I2C_REG_TB_U8_PWR_TMPON)==0 )
		{
			HAL_GPIO_WritePin(O_LED2_GPIO_Port,O_LED2_Pin,GPIO_PIN_RESET);

			HAL_SuspendTick();
			HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
			HAL_ResumeTick();

			/*
			 * ... Sleeping ...
			 */
			HAL_GPIO_WritePin(O_LED2_GPIO_Port,O_LED2_Pin,GPIO_PIN_SET);
			pm_loop_internal();
		}
		/*
		 * Wakeup ...
		 */
		SystemClock_Config();
		//HAL_SPI_Init(&hspi1);
		HAL_GPIO_WritePin(O_LED2_GPIO_Port,O_LED2_Pin,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(ZUMO_SHDN_GPIO_Port,ZUMO_SHDN_Pin,GPIO_PIN_RESET);

		ui_init();
#endif
	}
}
