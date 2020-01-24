/*
 * ui.c
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

#include "5x5_font.h"

#include "ui.h"

#include "ILI9341_Touchscreen.h"
#include "ILI9341_STM32_Driver.h"
#include "ILI9341_GFX.h"

#define UI_W	320
#define UI_H	240
#define UI_BG_COLOR		BLACK
#define UI_FG_COLOR_1	WHITE
#define UI_CHAR_SIZE	2
#define UI_CHAR_WIDTH 	(UI_CHAR_SIZE*CHAR_WIDTH)
#define UI_CHAR_HEIGHT 	(UI_CHAR_SIZE*CHAR_HEIGHT)

#define ADC_CONVERTED_DATA_BUFFER_SIZE 5
__IO   uint16_t   aADCxConvertedData[ADC_CONVERTED_DATA_BUFFER_SIZE]; /* ADC group regular conversion data (array of data) */

static uint32_t ui_time()
{
	return HAL_GetTick();
}

enum { POWER_OFF, POWER_ON, POWER_AUTO } power_mode = POWER_ON;
uint8_t power_mode_changed = 1;
uint32_t power_auto_offtimeout = 0;

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
	if( KEY2_Pin == GPIO_Pin )
	{
		switch( power_mode )
		{
		case POWER_OFF:
			power_mode = POWER_ON;
			power_mode_changed = 1;
			break;
		case POWER_ON:
			power_mode = POWER_AUTO;
			power_mode_changed = 1;
			break;
		case POWER_AUTO:
			power_mode = POWER_OFF;
			power_mode_changed = 1;
			power_auto_offtimeout = 0;
			break;
		default:
			power_mode = POWER_OFF;
			power_mode_changed = 1;
			break;
		}
	}
}

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

uint8_t ui_sleepmode()
{
	return (power_mode==POWER_OFF)?1:0;
}

void ui_update(uint8_t bInit)
{
	static uint32_t t_ = 0;
	uint32_t t = ui_time();
	char tmpstr[256];

	if( (t<t_) && (bInit==0) )
	{
		return;
	}
	t_ = t + 500;

	if( bInit )
	{
		ILI9341_Fill_Screen(UI_BG_COLOR);
		ILI9341_Draw_Horizontal_Line(0, 3*UI_CHAR_HEIGHT, UI_W, UI_FG_COLOR_1);
	}

	{
		RTC_DateTypeDef sdatestructureget;
		RTC_TimeTypeDef stimestructureget;
		/* Get the RTC current Time */
		HAL_RTC_GetTime(&hrtc, &stimestructureget, RTC_FORMAT_BIN);
		/* Get the RTC current Date */
		HAL_RTC_GetDate(&hrtc, &sdatestructureget, RTC_FORMAT_BIN);

		sprintf((char *)tmpstr, "Up:%03d.%02d.%02d.%02d:%03d ",
				/*t*/(int)((t/(1000*60*60*24))),
				/*h*/(int)((t/(1000*60*60))%60),
				/*m*/(int)((t/(1000*60))%60),
				/*s*/(int)((t/(1000))%60),
				/*ms*/(int)((t/(1))%1000));
		ILI9341_Draw_Text(tmpstr, 0, 0*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);
		sprintf((char *)tmpstr, "RTC:%02d-%02d-%04d",
				sdatestructureget.Month, sdatestructureget.Date, 2000 + sdatestructureget.Year);
		ILI9341_Draw_Text(tmpstr, 0, 1*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);
		sprintf((char *)tmpstr, "    %02d:%02d:%02d",
				stimestructureget.Hours, stimestructureget.Minutes, stimestructureget.Seconds);
		ILI9341_Draw_Text(tmpstr, 0, 2*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);
	}

	{
		ILI9341_Draw_Text("PWR:", UI_W-4*UI_CHAR_WIDTH, 1*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);
		if( power_mode == POWER_OFF )
		{
			ILI9341_Draw_Text(" OFF", UI_W-4*UI_CHAR_WIDTH, 2*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);
		}
		else if( power_mode == POWER_ON )
		{
			ILI9341_Draw_Text("  ON", UI_W-4*UI_CHAR_WIDTH, 2*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);
		}
		else if( power_mode == POWER_AUTO )
		{
			ILI9341_Draw_Text("AUTO", UI_W-4*UI_CHAR_WIDTH, 2*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);
		}
		else
		{
			ILI9341_Draw_Text("----", UI_W-4*UI_CHAR_WIDTH, 2*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);
		}
	}

#if 1
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
      /* Computation of ADC conversions raw data to physical values           */
      /* using LL ADC driver helper macro.                                    */
      /* Note: ADC results are transferred into array "aADCxConvertedData"  */
      /*       in the order of their rank in ADC sequencer.                   */
      {
    	  int r = 3;
    	  int u;

		  uint16_t vdda = __LL_ADC_CALC_VREFANALOG_VOLTAGE( aADCxConvertedData[3], LL_ADC_RESOLUTION_12B);

    	  int Rx = 21;
    	  u = ((100+Rx)*(uint32_t)__LL_ADC_CALC_DATA_TO_VOLTAGE(vdda, aADCxConvertedData[1], LL_ADC_RESOLUTION_12B)/Rx);
    	  sprintf(tmpstr,"UBAT:%4d",(int)u);
    	  if( u > (4*1200) )
    	  {
        	  ILI9341_Draw_Text(tmpstr, 0, 1+(r++)*UI_CHAR_HEIGHT, GREEN, UI_CHAR_SIZE, UI_BG_COLOR);
    	  }
    	  else if( u > (4*1100) )
    	  {
        	  ILI9341_Draw_Text(tmpstr, 0, 1+(r++)*UI_CHAR_HEIGHT, YELLOW, UI_CHAR_SIZE, UI_BG_COLOR);
    	  }
    	  else
    	  {
        	  ILI9341_Draw_Text(tmpstr, 0, 1+(r++)*UI_CHAR_HEIGHT, RED, UI_CHAR_SIZE, UI_BG_COLOR);
    	  }

    	  sprintf(tmpstr,"UCH:%4d",(int)((100+Rx)*(uint32_t)__LL_ADC_CALC_DATA_TO_VOLTAGE(vdda, aADCxConvertedData[0], LL_ADC_RESOLUTION_12B)/Rx));
    	  ILI9341_Draw_Text(tmpstr, 0, 1+(r++)*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);

    	  sprintf(tmpstr,"Temp:%2d",(int)__LL_ADC_CALC_TEMPERATURE(vdda, aADCxConvertedData[2], LL_ADC_RESOLUTION_12B));
    	  ILI9341_Draw_Text(tmpstr, 0, 1+(r++)*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);
      }

      /* Update status variable of DMA transfer */
      ubDmaTransferStatus = 0;
    }

#endif
#if 0
	float vdda = 3.0;

#if 0
	HAL_ADC_Start(&hadc1);
	HAL_ADCEx_Calibration_Start(&hadc1);

	{
		ADC_ChannelConfTypeDef   sConfig;
		sConfig.Channel      = ADC_CHANNEL_VREFINT;    /* ADC channel selection */
		sConfig.Rank         = ADC_REGULAR_RANK_1;        /* ADC group regular rank in which is mapped the selected ADC channel */
		sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1; /* ADC channel sampling time */

		if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) == HAL_OK)
		{
			if( HAL_ADC_PollForConversion(&hadc1,HAL_MAX_DELAY) == HAL_OK )
			{
				vdda = ((3.0 * HAL_ADCEx_Calibration_GetValue(&hadc1)) / HAL_ADC_GetValue(&hadc1));
				ILI9341_Draw_Text("Vdda:", 0, 1+3*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);
				sprintf(tmpstr,"%f ",vdda);
				ILI9341_Draw_Text(tmpstr, 0, 1+4*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);
			}
			else
			{
				ADC_ConversionStop(&hadc1);
			}
		}
	}

	HAL_ADC_Stop(&hadc1);
#endif

	HAL_ADC_Start(&hadc1);

	{
		ADC_ChannelConfTypeDef   sConfig;
		sConfig.Channel      = ADC_CHANNEL_0;		  /* ADC channel selection */
		sConfig.Rank         = ADC_REGULAR_RANK_1;        /* ADC group regular rank in which is mapped the selected ADC channel */
		sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1; /* ADC channel sampling time */

		if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) == HAL_OK)
		{
			if( HAL_ADC_PollForConversion(&hadc1,HAL_MAX_DELAY) == HAL_OK )
			{
				uint32_t v = (int)((1000.0 * vdda * HAL_ADC_GetValue(&hadc1)) / 4095);
				sprintf(tmpstr,"% 4d",(int)v);
				ILI9341_Draw_Text("Ubat:", 0, 1+5*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);
				ILI9341_Draw_Text(tmpstr, 0, 1+6*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);
			}
			else
			{
				ADC_ConversionStop(&hadc1);
			}
		}
	}

	HAL_ADC_Stop(&hadc1);
	HAL_ADC_Start(&hadc1);

	{
		ADC_ChannelConfTypeDef   sConfig;
		sConfig.Channel      = ADC_CHANNEL_VBAT;          /* ADC channel selection */
		sConfig.Rank         = ADC_REGULAR_RANK_1;        /* ADC group regular rank in which is mapped the selected ADC channel */
		sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1; /* ADC channel sampling time */

		if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) == HAL_OK)
		{
			if( HAL_ADC_PollForConversion(&hadc1,HAL_MAX_DELAY) == HAL_OK )
			{
				uint32_t v = (int)((1000.0 * vdda * HAL_ADC_GetValue(&hadc1)) / 4095);
				ILI9341_Draw_Text("VBat:", 0, 1+7*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);
				sprintf(tmpstr,"%d ",(int)v);
				ILI9341_Draw_Text(tmpstr, 0, 1+8*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);
			}
			else
			{
				ADC_ConversionStop(&hadc1);
			}
		}
	}

	HAL_ADC_Stop(&hadc1);
#if 1
	HAL_ADC_Start(&hadc1);

#define ADDR_TS_CAL1 ((uint16_t*) ((uint32_t)0x1FFF75A8)) /* 30 deg */
#define ADDR_TS_CAL2 ((uint16_t*) ((uint32_t)0x1FFF75CA)) /* 110 deg */

	{
		ADC_ChannelConfTypeDef   sConfig;
		sConfig.Channel      = ADC_CHANNEL_TEMPSENSOR;          /* ADC channel selection */
		sConfig.Rank         = ADC_REGULAR_RANK_1;        /* ADC group regular rank in which is mapped the selected ADC channel */
		sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_2; /* ADC channel sampling time */

		if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) == HAL_OK)
		{
			if( HAL_ADC_PollForConversion(&hadc1,HAL_MAX_DELAY) == HAL_OK )
			{
				float v = HAL_ADC_GetValue(&hadc1);
				v = (1.0*((110.0-30.0)/((*ADDR_TS_CAL2) - (*ADDR_TS_CAL1)))) * (v-(*ADDR_TS_CAL1)) + 30.0;
				ILI9341_Draw_Text("Temp:", 0, 1+9*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);
				sprintf(tmpstr,"%f ",v);
				ILI9341_Draw_Text(tmpstr, 0, 1+10*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);
			}
			else
			{
				ADC_ConversionStop(&hadc1);
			}
		}
	}

	HAL_ADC_Stop(&hadc1);
#endif
#endif

}


void ui_init()
{
	ILI9341_Init();	//initial driver setup to drive ili9341
	ILI9341_Fill_Screen(UI_BG_COLOR);
	ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);

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

	ui_update(1);
}

void ui_exit()
{
}

void ui_loop()
{
	ui_update(0);




#if 0
	ILI9341_Fill_Screen(WHITE);
	ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
	ILI9341_Draw_Text("FPS TEST, 40 loop 2 screens", 10, 10, BLACK, 1, WHITE);
	HAL_Delay(2000);
	ILI9341_Fill_Screen(WHITE);

	uint32_t Timer_Counter = 0;
	for(uint32_t j = 0; j < 2; j++)
	{
		HAL_TIM_Base_Start(&htim1);
		for(uint16_t i = 0; i < 10; i++)
		{
			ILI9341_Fill_Screen(WHITE);
			ILI9341_Fill_Screen(BLACK);
		}

		//20.000 per second!
		HAL_TIM_Base_Stop(&htim1);
		Timer_Counter += __HAL_TIM_GET_COUNTER(&htim1);
		__HAL_TIM_SET_COUNTER(&htim1, 0);
	}
	Timer_Counter /= 2;

#if 1
	char counter_buff[30];
	ILI9341_Fill_Screen(WHITE);
	ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
	sprintf(counter_buff, "Timer counter value: %d", Timer_Counter*2);
	ILI9341_Draw_Text(counter_buff, 10, 10, BLACK, 1, WHITE);

	double seconds_passed = 2*((float)Timer_Counter / 20000);
	sprintf(counter_buff, "Time: %.3f Sec", seconds_passed);
	ILI9341_Draw_Text(counter_buff, 10, 30, BLACK, 2, WHITE);

	double timer_float = 20/(((float)Timer_Counter)/20000);	//Frames per sec

	sprintf(counter_buff, "FPS:  %.2f", timer_float);
	ILI9341_Draw_Text(counter_buff, 10, 50, BLACK, 2, WHITE);
	double MB_PS = timer_float*240*320*2/1000000;
	sprintf(counter_buff, "MB/S: %.2f", MB_PS);
	ILI9341_Draw_Text(counter_buff, 10, 70, BLACK, 2, WHITE);
	double SPI_utilized_percentage = (MB_PS/(6.25 ))*100;		//50mbits / 8 bits
	sprintf(counter_buff, "SPI Utilized: %.2f", SPI_utilized_percentage);
	ILI9341_Draw_Text(counter_buff, 10, 90, BLACK, 2, WHITE);
	HAL_Delay(10000);


	static uint16_t x = 0;
	static uint16_t y = 0;

	char Temp_Buffer_text[40];

	//----------------------------------------------------------COUNTING MULTIPLE SEGMENTS
	ILI9341_Fill_Screen(WHITE);
	ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
	ILI9341_Draw_Text("Counting multiple segments at once", 10, 10, BLACK, 1, WHITE);
	HAL_Delay(2000);
	ILI9341_Fill_Screen(WHITE);


	for(uint16_t i = 0; i <= 10; i++)
	{
		sprintf(Temp_Buffer_text, "Counting: %d", i);
		ILI9341_Draw_Text(Temp_Buffer_text, 10, 10, BLACK, 2, WHITE);
		ILI9341_Draw_Text(Temp_Buffer_text, 10, 30, BLUE, 2, WHITE);
		ILI9341_Draw_Text(Temp_Buffer_text, 10, 50, RED, 2, WHITE);
		ILI9341_Draw_Text(Temp_Buffer_text, 10, 70, GREEN, 2, WHITE);
		ILI9341_Draw_Text(Temp_Buffer_text, 10, 90, BLACK, 2, WHITE);
		ILI9341_Draw_Text(Temp_Buffer_text, 10, 110, BLUE, 2, WHITE);
		ILI9341_Draw_Text(Temp_Buffer_text, 10, 130, RED, 2, WHITE);
		ILI9341_Draw_Text(Temp_Buffer_text, 10, 150, GREEN, 2, WHITE);
		ILI9341_Draw_Text(Temp_Buffer_text, 10, 170, WHITE, 2, BLACK);
		ILI9341_Draw_Text(Temp_Buffer_text, 10, 190, BLUE, 2, BLACK);
		ILI9341_Draw_Text(Temp_Buffer_text, 10, 210, RED, 2, BLACK);
	}

	HAL_Delay(1000);

	//----------------------------------------------------------COUNTING SINGLE SEGMENT
	ILI9341_Fill_Screen(WHITE);
	ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
	ILI9341_Draw_Text("Counting single segment", 10, 10, BLACK, 1, WHITE);
	HAL_Delay(2000);
	ILI9341_Fill_Screen(WHITE);

	for(uint16_t i = 0; i <= 100; i++)
	{
		sprintf(Temp_Buffer_text, "Counting: %d", i);
		ILI9341_Draw_Text(Temp_Buffer_text, 10, 10, BLACK, 3, WHITE);
	}

	HAL_Delay(1000);

	//----------------------------------------------------------ALIGNMENT TEST
	ILI9341_Fill_Screen(WHITE);
	ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
	ILI9341_Draw_Text("Rectangle alignment check", 10, 10, BLACK, 1, WHITE);
	HAL_Delay(2000);
	ILI9341_Fill_Screen(WHITE);

	ILI9341_Draw_Hollow_Rectangle_Coord(50, 50, 100, 100, BLACK);
	ILI9341_Draw_Filled_Rectangle_Coord(20, 20, 50, 50, BLACK);
	ILI9341_Draw_Hollow_Rectangle_Coord(10, 10, 19, 19, BLACK);
	HAL_Delay(1000);

	//----------------------------------------------------------LINES EXAMPLE
	ILI9341_Fill_Screen(WHITE);
	ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
	ILI9341_Draw_Text("Randomly placed and sized", 10, 10, BLACK, 1, WHITE);
	ILI9341_Draw_Text("Horizontal and Vertical lines", 10, 20, BLACK, 1, WHITE);
	HAL_Delay(2000);
	ILI9341_Fill_Screen(WHITE);

	for(uint32_t i = 0; i < 30000; i++)
	{
		uint32_t random_num = 0;
		uint16_t xr = 0;
		uint16_t yr = 0;
		uint16_t radiusr = 0;
		uint16_t colourr = 0;
		//random_num = HAL_RNG_GetRandomNumber(&hrng);
		HAL_RNG_GenerateRandomNumber(&hrng,&random_num);
		xr = random_num;
		//random_num = HAL_RNG_GetRandomNumber(&hrng);
		HAL_RNG_GenerateRandomNumber(&hrng,&random_num);
		yr = random_num;
		//random_num = HAL_RNG_GetRandomNumber(&hrng);
		HAL_RNG_GenerateRandomNumber(&hrng,&random_num);
		radiusr = random_num;
		//random_num = HAL_RNG_GetRandomNumber(&hrng);
		HAL_RNG_GenerateRandomNumber(&hrng,&random_num);
		colourr = random_num;

		xr &= 0x01FF;
		yr &= 0x01FF;
		radiusr &= 0x001F;
		//ili9341_drawpixel(xr, yr, WHITE);
		ILI9341_Draw_Horizontal_Line(xr, yr, radiusr, colourr);
		ILI9341_Draw_Vertical_Line(xr, yr, radiusr, colourr);
	}

	HAL_Delay(1000);

	//----------------------------------------------------------HOLLOW CIRCLES EXAMPLE
	ILI9341_Fill_Screen(WHITE);
	ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
	ILI9341_Draw_Text("Randomly placed and sized", 10, 10, BLACK, 1, WHITE);
	ILI9341_Draw_Text("Circles", 10, 20, BLACK, 1, WHITE);
	HAL_Delay(2000);
	ILI9341_Fill_Screen(WHITE);


	for(uint32_t i = 0; i < 3000; i++)
	{
		uint32_t random_num = 0;
		uint16_t xr = 0;
		uint16_t yr = 0;
		uint16_t radiusr = 0;
		uint16_t colourr = 0;
		//random_num = HAL_RNG_GetRandomNumber(&hrng);
		HAL_RNG_GenerateRandomNumber(&hrng,&random_num);
		xr = random_num;
		//random_num = HAL_RNG_GetRandomNumber(&hrng);
		HAL_RNG_GenerateRandomNumber(&hrng,&random_num);
		yr = random_num;
		//random_num = HAL_RNG_GetRandomNumber(&hrng);
		HAL_RNG_GenerateRandomNumber(&hrng,&random_num);
		radiusr = random_num;
		//random_num = HAL_RNG_GetRandomNumber(&hrng);
		HAL_RNG_GenerateRandomNumber(&hrng,&random_num);
		colourr = random_num;

		xr &= 0x01FF;
		yr &= 0x01FF;
		radiusr &= 0x001F;
		//ili9341_drawpixel(xr, yr, WHITE);
		ILI9341_Draw_Hollow_Circle(xr, yr, radiusr*2, colourr);
	}
	HAL_Delay(1000);

	//----------------------------------------------------------FILLED CIRCLES EXAMPLE
	ILI9341_Fill_Screen(WHITE);
	ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
	ILI9341_Draw_Text("Randomly placed and sized", 10, 10, BLACK, 1, WHITE);
	ILI9341_Draw_Text("Filled Circles", 10, 20, BLACK, 1, WHITE);
	HAL_Delay(2000);
	ILI9341_Fill_Screen(WHITE);

	for(uint32_t i = 0; i < 1000; i++)
	{
		uint32_t random_num = 0;
		uint16_t xr = 0;
		uint16_t yr = 0;
		uint16_t radiusr = 0;
		uint16_t colourr = 0;
		//random_num = HAL_RNG_GetRandomNumber(&hrng);
		HAL_RNG_GenerateRandomNumber(&hrng,&random_num);
		xr = random_num;
		//random_num = HAL_RNG_GetRandomNumber(&hrng);
		HAL_RNG_GenerateRandomNumber(&hrng,&random_num);
		yr = random_num;
		//random_num = HAL_RNG_GetRandomNumber(&hrng);
		HAL_RNG_GenerateRandomNumber(&hrng,&random_num);
		radiusr = random_num;
		//random_num = HAL_RNG_GetRandomNumber(&hrng);
		HAL_RNG_GenerateRandomNumber(&hrng,&random_num);
		colourr = random_num;

		xr &= 0x01FF;
		yr &= 0x01FF;
		radiusr &= 0x001F;
		//ili9341_drawpixel(xr, yr, WHITE);
		ILI9341_Draw_Filled_Circle(xr, yr, radiusr/2, colourr);
	}
	HAL_Delay(1000);

	//----------------------------------------------------------HOLLOW RECTANGLES EXAMPLE
	ILI9341_Fill_Screen(WHITE);
	ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
	ILI9341_Draw_Text("Randomly placed and sized", 10, 10, BLACK, 1, WHITE);
	ILI9341_Draw_Text("Rectangles", 10, 20, BLACK, 1, WHITE);
	HAL_Delay(2000);
	ILI9341_Fill_Screen(WHITE);

	for(uint32_t i = 0; i < 20000; i++)
	{
		uint32_t random_num = 0;
		uint16_t xr = 0;
		uint16_t yr = 0;
		uint16_t radiusr = 0;
		uint16_t colourr = 0;
		//random_num = HAL_RNG_GetRandomNumber(&hrng);
		HAL_RNG_GenerateRandomNumber(&hrng,&random_num);
		xr = random_num;
		//random_num = HAL_RNG_GetRandomNumber(&hrng);
		HAL_RNG_GenerateRandomNumber(&hrng,&random_num);
		yr = random_num;
		//random_num = HAL_RNG_GetRandomNumber(&hrng);
		HAL_RNG_GenerateRandomNumber(&hrng,&random_num);
		radiusr = random_num;
		//random_num = HAL_RNG_GetRandomNumber(&hrng);
		HAL_RNG_GenerateRandomNumber(&hrng,&random_num);
		colourr = random_num;

		xr &= 0x01FF;
		yr &= 0x01FF;
		radiusr &= 0x001F;
		//ili9341_drawpixel(xr, yr, WHITE);
		ILI9341_Draw_Hollow_Rectangle_Coord(xr, yr, xr+radiusr, yr+radiusr, colourr);
	}
	HAL_Delay(1000);

	//----------------------------------------------------------FILLED RECTANGLES EXAMPLE
	ILI9341_Fill_Screen(WHITE);
	ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
	ILI9341_Draw_Text("Randomly placed and sized", 10, 10, BLACK, 1, WHITE);
	ILI9341_Draw_Text("Filled Rectangles", 10, 20, BLACK, 1, WHITE);
	HAL_Delay(2000);
	ILI9341_Fill_Screen(WHITE);

	for(uint32_t i = 0; i < 20000; i++)
	{
		uint32_t random_num = 0;
		uint16_t xr = 0;
		uint16_t yr = 0;
		uint16_t radiusr = 0;
		uint16_t colourr = 0;
		//random_num = HAL_RNG_GetRandomNumber(&hrng);
		HAL_RNG_GenerateRandomNumber(&hrng,&random_num);
		xr = random_num;
		//random_num = HAL_RNG_GetRandomNumber(&hrng);
		HAL_RNG_GenerateRandomNumber(&hrng,&random_num);
		yr = random_num;
		//random_num = HAL_RNG_GetRandomNumber(&hrng);
		HAL_RNG_GenerateRandomNumber(&hrng,&random_num);
		radiusr = random_num;
		//random_num = HAL_RNG_GetRandomNumber(&hrng);
		HAL_RNG_GenerateRandomNumber(&hrng,&random_num);
		colourr = random_num;

		xr &= 0x01FF;
		yr &= 0x01FF;
		radiusr &= 0x001F;
		//ili9341_drawpixel(xr, yr, WHITE);
		ILI9341_Draw_Rectangle(xr, yr, radiusr, radiusr, colourr);
	}
	HAL_Delay(1000);

	//----------------------------------------------------------INDIVIDUAL PIXEL EXAMPLE

	ILI9341_Fill_Screen(WHITE);
	ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
	ILI9341_Draw_Text("Slow draw by selecting", 10, 10, BLACK, 1, WHITE);
	ILI9341_Draw_Text("and adressing pixels", 10, 20, BLACK, 1, WHITE);
	HAL_Delay(2000);
	ILI9341_Fill_Screen(WHITE);

	x = 0;
	y = 0;
	while (y < 240)
	{
		while ((x < 320) && (y < 240))
		{

			if(x % 2)
			{
				ILI9341_Draw_Pixel(x, y, BLACK);
			}

			x++;
		}

		y++;
		x = 0;
	}

	x = 0;
	y = 0;


	while (y < 240)
	{
		while ((x < 320) && (y < 240))
		{

			if(y % 2)
			{
				ILI9341_Draw_Pixel(x, y, BLACK);
			}

			x++;
		}

		y++;
		x = 0;
	}
	HAL_Delay(2000);

	//----------------------------------------------------------INDIVIDUAL PIXEL EXAMPLE
	ILI9341_Fill_Screen(WHITE);
	ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
	ILI9341_Draw_Text("Random position and colour", 10, 10, BLACK, 1, WHITE);
	ILI9341_Draw_Text("500000 pixels", 10, 20, BLACK, 1, WHITE);
	HAL_Delay(2000);
	ILI9341_Fill_Screen(WHITE);


	for(uint32_t i = 0; i < 500000; i++)
	{
		uint32_t random_num = 0;
		uint32_t color = 0;
		uint16_t xr = 0;
		uint16_t yr = 0;
		//random_num = HAL_RNG_GetRandomNumber(&hrng);
		HAL_RNG_GenerateRandomNumber(&hrng,&random_num);
		xr = random_num;
		//random_num = HAL_RNG_GetRandomNumber(&hrng);
		HAL_RNG_GenerateRandomNumber(&hrng,&random_num);
		yr = random_num;
		//uint16_t color = HAL_RNG_GetRandomNumber(&hrng);
		HAL_RNG_GenerateRandomNumber(&hrng,&color);

		xr &= 0x01FF;
		yr &= 0x01FF;
		ILI9341_Draw_Pixel(xr, yr, color);
	}
	HAL_Delay(2000);

	//----------------------------------------------------------565 COLOUR EXAMPLE, Grayscale
	ILI9341_Fill_Screen(WHITE);
	ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
	ILI9341_Draw_Text("Colour gradient", 10, 10, BLACK, 1, WHITE);
	ILI9341_Draw_Text("Grayscale", 10, 20, BLACK, 1, WHITE);
	HAL_Delay(2000);


	for(uint16_t i = 0; i <= (320); i++)
	{
		uint16_t Red = 0;
		uint16_t Green = 0;
		uint16_t Blue = 0;

		Red = i/(10);
		Red <<= 11;
		Green = i/(5);
		Green <<= 5;
		Blue = i/(10);



		uint16_t RGB_color = Red + Green + Blue;
		ILI9341_Draw_Rectangle(i, x, 1, 240, RGB_color);

	}
	HAL_Delay(2000);

	//----------------------------------------------------------TOUCHSCREEN EXAMPLE
	ILI9341_Fill_Screen(WHITE);
	ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);
	ILI9341_Draw_Text("Touchscreen", 10, 10, BLACK, 2, WHITE);
	ILI9341_Draw_Text("Touch to draw", 10, 30, BLACK, 2, WHITE);
	ILI9341_Set_Rotation(SCREEN_VERTICAL_1);
#endif
#endif

}

