/*
 * pm.c
 *
 *  Created on: Jan 16, 2020
 *      Author: maik
 */
#include <stdio.h>

#include "config.h"
#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"
#ifdef HAL_IWDG_MODULE_ENABLED
#include "iwdg.h"
#endif
#include "lptim.h"

#include "pm.h"

uint32_t charger_state = 0;

#ifdef ENABLE_STANDBY
void vStandby()
{
	HAL_ADC_Stop_DMA(&hadc1);

	HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1,charger_state);

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
#endif

#define ADC_CONVERTED_DATA_BUFFER_SIZE 8
__IO   uint16_t   aADCxConvertedData[ADC_CONVERTED_DATA_BUFFER_SIZE]; /* ADC group regular conversion data (array of data) */
volatile uint64_t aADCxCnt=0;
volatile uint32_t aADCxTimeStamp=0;
volatile uint32_t aADCxRate=0;
#if 1
//uint8_t ubDmaTransferStatus = 0;
/**
  * @brief  Conversion complete callback in non blocking mode
  * @param  hadc: ADC handle
  * @note   This example shows a simple way to report end of conversion
  *         and get conversion result. You can add your own implementation.
  * @retval None
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
#if 1
	uint32_t t = HAL_GetTick();
	aADCxRate = t - aADCxTimeStamp;
	aADCxTimeStamp = t;
#endif

#ifdef ENABLE_IRQ_ADC
	int i;
	uint32_t sum_vdda = 0;
	uint32_t sum_ucharge = 0;
	uint32_t sum_ubat = 0;
	uint32_t sum_temp = 0;

	sum_vdda += aADCxConvertedData[3];
	sum_ubat += aADCxConvertedData[1];
	sum_ucharge += aADCxConvertedData[0];
	sum_temp += aADCxConvertedData[2];

	{
		int v;
		uint16_t vdda = __LL_ADC_CALC_VREFANALOG_VOLTAGE( sum_vdda, LL_ADC_RESOLUTION_12B);

		int Rx = 22;
		v = ((100+Rx)*(uint32_t)__LL_ADC_CALC_DATA_TO_VOLTAGE(vdda, sum_ubat, LL_ADC_RESOLUTION_12B)/Rx);
		v = v*100/118;
		API_I2C1_u16Set(I2C_REG_TB_U16_UBAT_MV,v);

		{
			uint32_t ubat_mean = API_I2C1_u16Get(I2C_REG_TB_U16_UBAT_MEAN_MV);
			ubat_mean = (9*ubat_mean+v)/10;
			API_I2C1_u16Set(I2C_REG_TB_U16_UBAT_MEAN_MV,ubat_mean);
		}

		v = (int)((100+Rx)*(uint32_t)__LL_ADC_CALC_DATA_TO_VOLTAGE(vdda, sum_ucharge, LL_ADC_RESOLUTION_12B)/Rx);
		v = v*100/118;
		API_I2C1_u16Set(I2C_REG_TB_U16_UCHARGE_MV,v);

		v = (int)__LL_ADC_CALC_TEMPERATURE(vdda, sum_temp, LL_ADC_RESOLUTION_12B);
		API_I2C1_u16Set(I2C_REG_TB_U16_TEMP_C,v);
	}
#endif
	aADCxCnt++;
}

/**
  * @brief  Conversion DMA half-transfer callback in non blocking mode
  * @note   This example shows a simple way to report end of conversion
  *         and get conversion result. You can add your own implementation.
  * @retval None
  */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
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

#ifndef ENABLE_IRQ_ADC
void pm_run_adc()
{
	int i;
	int n = 1;
	uint32_t sum_vdda = 0;
	uint32_t sum_ucharge = 0;
	uint32_t sum_ubat = 0;
	uint32_t sum_temp = 0;

	if (HAL_ADC_Start(&hadc1) == HAL_OK)
	{
		if( HAL_ADC_PollForConversion(&hadc1,10) == HAL_OK )
		{
			sum_vdda += aADCxConvertedData[3];
			sum_ubat += aADCxConvertedData[1];
			sum_ucharge += aADCxConvertedData[0];
			sum_temp += aADCxConvertedData[2];

			{
				int v;
				uint16_t vdda = __LL_ADC_CALC_VREFANALOG_VOLTAGE( sum_vdda/n, LL_ADC_RESOLUTION_12B);

				int Rx = 22;
				v = ((100+Rx)*(uint32_t)__LL_ADC_CALC_DATA_TO_VOLTAGE(vdda, sum_ubat/n, LL_ADC_RESOLUTION_12B)/Rx);
				v = v*100/118;
				API_I2C1_u16Set(I2C_REG_TB_U16_UBAT_MV,v);

				{
					uint32_t ubat_mean = API_I2C1_u16Get(I2C_REG_TB_U16_UBAT_MEAN_MV);
					ubat_mean = (9*ubat_mean+v)/10;
					API_I2C1_u16Set(I2C_REG_TB_U16_UBAT_MEAN_MV,ubat_mean);
				}

				v = (int)((100+Rx)*(uint32_t)__LL_ADC_CALC_DATA_TO_VOLTAGE(vdda, sum_ucharge/n, LL_ADC_RESOLUTION_12B)/Rx);
				v = v*100/118;
				API_I2C1_u16Set(I2C_REG_TB_U16_UCHARGE_MV,v);

				v = (int)__LL_ADC_CALC_TEMPERATURE(vdda, sum_temp/n, LL_ADC_RESOLUTION_12B);
				API_I2C1_u16Set(I2C_REG_TB_U16_TEMP_C,v);
			}

			HAL_ADC_Stop(&hadc1);
		}
	}
}
#endif

uint16_t ubat_nocharge_mean_mv = 0;
uint16_t ubat_mean_mv = 0;
uint16_t ucharge_mean_mv = 0;
int8_t ubat_nocharge_dir = 0;
uint8_t bat_level = 0;
void pm_charge_step()
{
	uint8_t _state = (charger_state>>31)&0x1;	// 1 bit
	uint8_t _decharging = (charger_state>>25)&0x3f;	// 6 bit
	uint8_t _fullcnt = (charger_state>>22)&0x3f;	// 3 bit
	uint8_t _seconds = (charger_state>>16)&0x3f; // 6 bit
	uint16_t _ubatprev = (charger_state>>0)&0xffff; // 16 bit

	static uint64_t aADCxCntPrev=0;
	if( aADCxCntPrev != 0 )
	{
		if( aADCxCntPrev == aADCxCnt )
		{
			/*
			 * ADC is not active, disable any charging off
			 */
#ifdef O_LED2_Pin
			HAL_GPIO_WritePin(O_LED2_GPIO_Port,O_LED2_Pin,GPIO_PIN_RESET);
#endif
			HAL_GPIO_WritePin(O_CHARGE_ON_GPIO_Port,O_CHARGE_ON_Pin,GPIO_PIN_RESET);
			return;
		}
		aADCxCntPrev = aADCxCnt;
	}

	uint16_t u_bat = API_I2C1_u16Get(I2C_REG_TB_U16_UBAT_MV);
	uint16_t u_charge = API_I2C1_u16Get(I2C_REG_TB_U16_UCHARGE_MV);

	ubat_mean_mv = (3*ubat_mean_mv + u_bat)/4;
	ucharge_mean_mv = (3*ucharge_mean_mv + u_charge)/4;

	uint8_t charge_current_on = ( HAL_GPIO_ReadPin(O_CHARGE_ON_GPIO_Port,O_CHARGE_ON_Pin) == GPIO_PIN_RESET )?0:1;

	if(( _decharging == 0 ) || ( _ubatprev < UBAT_CHARGE_A ))
	{
		/*
		 * charging ...
		 */
		if(( (_seconds++ % CHARGE_ON_PRERIOD) < CHARGE_ON_TIME) ||
				( u_charge < U_CHARGE_MIN ) ||
				( u_charge > U_CHARGE_MAX ) ||
				( u_bat < UBAT_MIN_CHARGE ) ||
				( u_bat > UBAT_MAX ) ||
				( charge_current_on==1 && ((u_charge-u_bat) > U_CHARGE_MAX_DIFF) ) || /* limit the charge current */
				(u_charge < u_bat))
		{
			/*
			 * no charging ...
			 */
			if( charge_current_on == 0 )
			{
				_ubatprev = u_bat;
				ubat_nocharge_mean_mv = (3*ubat_nocharge_mean_mv + u_bat)/4;
			}
#ifdef O_LED2_Pin
			HAL_GPIO_WritePin(O_LED2_GPIO_Port,O_LED2_Pin,GPIO_PIN_RESET);
#endif
			HAL_GPIO_WritePin(O_CHARGE_ON_GPIO_Port,O_CHARGE_ON_Pin,GPIO_PIN_RESET);
		}
		else
		{
			/*
			 * charging ...
			 */
			if( charge_current_on == 0 )
			{
				API_I2C1_u16Set(I2C_REG_TB_U16_UBAT_IDLE_MV,u_bat);
				/*
				 * check voltage trend ...
				 */
				if( u_bat < _ubatprev )
				{
					if(ubat_nocharge_dir > -9)
					{
						ubat_nocharge_dir--;
					}
				}
				else
				{
					if(ubat_nocharge_dir < 9)
					{
						ubat_nocharge_dir++;
					}
				}
				_ubatprev = u_bat;

				uint8_t _bat_level = 0;
				if( u_bat > UBAT_MIN )
				{
					_bat_level = 100 * (u_bat - UBAT_MIN) / (UBAT_MAX - UBAT_MIN);
				}
				bat_level = (3*bat_level+_bat_level)/4;

				API_I2C1_u16Set(I2C_REG_TB_U16_UBAT_LEVEL,(uint16_t)bat_level);
				API_I2C1_u16Set(I2C_REG_TB_S16_UBAT_DIR,(uint16_t)ubat_nocharge_dir);
			}
			else
			{
				API_I2C1_u16Set(I2C_REG_TB_U16_UBAT_CHARGE_MV,u_bat);
			}

			/*
			 * switch charging on ...
			 */
#ifdef O_LED2_Pin
			HAL_GPIO_WritePin(O_LED2_GPIO_Port,O_LED2_Pin,GPIO_PIN_SET);
#endif
			HAL_GPIO_WritePin(O_CHARGE_ON_GPIO_Port,O_CHARGE_ON_Pin,GPIO_PIN_SET);
		}
	}

	charger_state = 0;
	charger_state |= _state<<31;
	charger_state |= _decharging<<25;
	charger_state |= _fullcnt<<22;
	charger_state |= _seconds<<16;
	charger_state |= _ubatprev<<0;
}

#ifdef ENABLE_LCD_UI
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
			API_I2C1_u8Set(I2C_REG_TB_U8_PWRMODE_NEXT,PWRMODE_AUTO);
			API_I2C1_u8Set(I2C_REG_TB_U8_PWRCNTDWN,PWRCNTDWN_START);
			break;
		case PWRMODE_AUTO:
			API_I2C1_u8Set(I2C_REG_TB_U8_PWRMODE_NEXT,PWRMODE_ON);
			API_I2C1_u8Set(I2C_REG_TB_U8_PWRCNTDWN,PWRCNTDWN_START);
			break;
		case PWRMODE_ON:
			API_I2C1_u8Set(I2C_REG_TB_U8_PWRMODE_NEXT,PWRMODE_OFF);
			API_I2C1_u8Set(I2C_REG_TB_U8_PWRCNTDWN,PWRCNTDWN_START);
			break;
		}

		API_I2C1_u8Set(I2C_REG_TB_U8_PWR_TMPON,PWR_TMPON_TIME);
		HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0,0x42affe00|API_I2C1_u8Get(I2C_REG_TB_U8_PWRMODE_NEXT));
	}

#if 0
	if( KEY1_Pin == GPIO_Pin )
	{
		API_I2C1_u8Set(I2C_REG_TB_U8_PWRMODE_NEXT,PWRMODE_OFF);
		API_I2C1_u8Set(I2C_REG_TB_U8_PWRCNTDWN,PWRCNTDWN_START);
		API_I2C1_u8Set(I2C_REG_TB_U8_PWR_TMPON,PWR_TMPON_TIME);
		HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0,0x42affe00|API_I2C1_u8Get(I2C_REG_TB_U8_PWRMODE_NEXT));
	}
#endif

	if( KEY0_Pin == GPIO_Pin )
	{
#ifdef ENABLE_STANDBY
			vStandby();
#endif
	}
}
#endif

void pm_early_init()
{
	aADCxCnt=0;
}

void pm_init()
{
	HAL_LPTIM_TimeOut_Start_IT(&hlptim1,1000,1000);

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

	API_I2C1_u8Set(I2C_REG_TB_U8_PWRCNTDWN,PWRCNTDWN_START);
	API_I2C1_u8Set(I2C_REG_TB_U8_PWRMODE_NEXT,PWRMODE_AUTO);
	API_I2C1_u8Set(I2C_REG_TB_U8_PWR_TMPON,PWR_TMPON_TIME);
    API_I2C1_u8Set(I2C_REG_TB_U8_PWRMODE,HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0)&0xff);

	API_I2C1_u16Set(I2C_REG_TB_U16_TON_TOUT,0);
	API_I2C1_u16Set(I2C_REG_TB_U16_TON_PERIOD,5);
	API_I2C1_u16Set(I2C_REG_TB_U16_TON_WDG,0);
	API_I2C1_u16Set(I2C_REG_TB_U16_TOFF_TOUT,0);
	API_I2C1_u16Set(I2C_REG_TB_U16_TOFF_PERIOD,5*60);
	API_I2C1_u16Set(I2C_REG_TB_U16_TON_TOUT,API_I2C1_u16Get(I2C_REG_TB_U16_TON_PERIOD));
	//__HAL_RCC_PWR_CLK_ENABLE();
}

void pm_exit()
{

}

uint8_t handle_charge = 0;
void HAL_LPTIM_CompareMatchCallback(LPTIM_HandleTypeDef *hlptim)
{
	handle_charge = 1;
}

void pm_loop()
{
	uint8_t tick_1s = 0;
	uint8_t pm = API_I2C1_u8Get(I2C_REG_TB_U8_PWRMODE);

#ifdef HAL_IWDG_MODULE_ENABLED
	HAL_IWDG_Refresh(&hiwdg);
#endif

	if( aADCxCnt > 1 )
	{
		if( handle_charge == 1 )
		{
			handle_charge = 0;
			pm_charge_step();
			tick_1s = 1;
		}
	}

	if( tick_1s == 1 )
	{
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
			else
			{
				API_I2C1_u8Set(I2C_REG_TB_U8_PWRMODE,API_I2C1_u8Get(I2C_REG_TB_U8_PWRMODE_NEXT));
				API_I2C1_u8Set(I2C_REG_TB_U8_PWRMODE_NEXT,PWRMODE_UNDEF);
			}
		}

		if( API_I2C1_u8Get(I2C_REG_TB_U8_PWR_TMPON) != 0 )
		{
			API_I2C1_u8Set(I2C_REG_TB_U8_PWR_TMPON,API_I2C1_u8Get(I2C_REG_TB_U8_PWR_TMPON)-1);
		}
	}

#if 0
	{
		uint16_t u_bat = API_I2C1_u16Get(I2C_REG_TB_U16_UBAT_MV);
#ifdef UBAT_OK
		if( (u_bat<UBAT_MIN) && (API_I2C1_u8Get(I2C_REG_TB_U8_PWR_TMPON)==0) )
		{
			/*
			 * auto shutdown when ubat is to low
			 */
			API_I2C1_u8Set(I2C_REG_TB_U8_PWRMODE_NEXT,PWRMODE_OFF);
			API_I2C1_u8Set(I2C_REG_TB_U8_PWRCNTDWN,PWRCNTDWN_START);
		}
#endif
	}
#endif

	{
		uint8_t zumo_on = 0;

		if( !(pm==PWRMODE_OFF || pm==PWRMODE_UNDEF) )
		{
			zumo_on = 1;
		}

		if(API_I2C1_u8Get(I2C_REG_TB_U8_PWR_TMPON)!=0)
		{
			zumo_on = 1;
		}

		if( pm == PWRMODE_AUTO )
		{
			uint16_t x = API_I2C1_u16Get(I2C_REG_TB_U16_TON_TOUT);
			if( x != 0 )
			{
				if(tick_1s==1) x--;
				API_I2C1_u16Set(I2C_REG_TB_U16_TON_TOUT,x);
				if( (x) == 0 )
				{
					zumo_on = 0;
				}
				else
				{
					zumo_on = 1;
				}
			}
			else if( API_I2C1_u16Get(I2C_REG_TB_U16_TOFF_TOUT) != 0 )
			{
				uint16_t x = API_I2C1_u16Get(I2C_REG_TB_U16_TOFF_TOUT);
				if( x != 0 )
				{
					if(tick_1s==1) x--;
					API_I2C1_u16Set(I2C_REG_TB_U16_TOFF_TOUT,x);
					if( (x) == 0 )
					{
						zumo_on = 1;
					}
					else
					{
						zumo_on = 0;
					}
				}
			}

			if( API_I2C1_u16Get(I2C_REG_TB_U16_TON_WDG) != 0 )
			{
				if(tick_1s==1) x--;
				API_I2C1_u16Set(I2C_REG_TB_U16_TON_WDG,x);
				if( (x) == 0 )
				{
					zumo_on = 0;
				}
			}
		}

		if( API_I2C1_u16Get(I2C_REG_TB_U16_UBAT_IDLE_MV) < UBAT_MIN )
		{
			zumo_on = 0;
		}

#ifndef ENABLE_LCD_UI
		if( HAL_GPIO_ReadPin(KEY2_GPIO_Port,KEY2_Pin) == GPIO_PIN_RESET ) /* keep zumo on when jumper is set*/
		{
			zumo_on = 1;
		}
#endif

		if(zumo_on != 0)
		{
			/*
			 * switch ZUMO on
			 */
			if( HAL_GPIO_ReadPin(ZUMO_SHDN_GPIO_Port,ZUMO_SHDN_Pin) != GPIO_PIN_RESET )
			{
		  	    GPIO_InitTypeDef GPIO_InitStruct = {0};
				GPIO_InitStruct.Pin = ZUMO_SHDN_Pin;
				GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
				GPIO_InitStruct.Pull = GPIO_PULLUP;
				HAL_GPIO_Init(ZUMO_SHDN_GPIO_Port, &GPIO_InitStruct);
				HAL_GPIO_WritePin(ZUMO_SHDN_GPIO_Port,ZUMO_SHDN_Pin,GPIO_PIN_RESET);

				API_I2C1_u16Set(I2C_REG_TB_U16_TON_TOUT,API_I2C1_u16Get(I2C_REG_TB_U16_TON_PERIOD));
			}
		}
		else
		{
			/*
			 * switch ZUMO off
			 */
			if( HAL_GPIO_ReadPin(ZUMO_SHDN_GPIO_Port,ZUMO_SHDN_Pin) != GPIO_PIN_SET )
			{
		  	    GPIO_InitTypeDef GPIO_InitStruct = {0};
				GPIO_InitStruct.Pin = ZUMO_SHDN_Pin;
				GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
				GPIO_InitStruct.Pull = GPIO_PULLUP;
				HAL_GPIO_Init(ZUMO_SHDN_GPIO_Port, &GPIO_InitStruct);
				HAL_GPIO_WritePin(ZUMO_SHDN_GPIO_Port,ZUMO_SHDN_Pin,GPIO_PIN_SET);
				HAL_GPIO_WritePin(GPIOC, RST_Pin, GPIO_PIN_RESET);

				API_I2C1_u16Set(I2C_REG_TB_U16_TOFF_TOUT,API_I2C1_u16Get(I2C_REG_TB_U16_TOFF_PERIOD));
			}
			//HAL_SPI_DeInit(&hspi1);

			API_I2C1_u8Set(I2C_REG_TB_U8_PWRMODE,0x42affe00|API_I2C1_u8Get(I2C_REG_TB_U8_PWRMODE));
			if( aADCxCnt > 2 )
			{
	#ifdef ENABLE_STANDBY
				vStandby();
	#endif
			}
		}
	}

#if 0
	//HAL_GPIO_WritePin(O_LED2_GPIO_Port,O_LED2_Pin,GPIO_PIN_RESET);
    /*Suspend Tick increment to prevent wakeup by Systick interrupt.
    Otherwise the Systick interrupt will wake up the device within 1ms (HAL time base)*/
    HAL_SuspendTick();

    /* Enter Sleep Mode , wake up is done once Tamper push-button is pressed */
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

    /* Resume Tick interrupt if disabled prior to SLEEP mode entry */
    HAL_ResumeTick();
	//HAL_GPIO_WritePin(O_LED2_GPIO_Port,O_LED2_Pin,GPIO_PIN_SET);
#endif
}
