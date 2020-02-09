/*
 * ui.c
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

#include "5x5_font.h"

#include "i2c.h"
#include "ui.h"
#include "pm.h"

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

static uint32_t ui_time()
{
	return HAL_GetTick();
}

static void ui_update_terminal()
{
	if( API_I2C1_u8WRFlag(I2C_REG_TB_U16_TERMINALBUFFER) != 0 )
	{
		int x = 0;
		int y = UI_H - CHAR_HEIGHT;
		uint16_t addr = I2C_REG_TB_U16_TERMINALBUFFER+I2C_TERMINAL_BUFFER_SIZE-1;
		uint16_t addr1 = 0;
		uint16_t addr2 = 0;
		uint8_t c;

		do
		{
			while( (c=API_I2C1_u8Get(addr))<32 )
			{
				addr--;
				if( addr<I2C_REG_TB_U16_TERMINALBUFFER )
				{
					return;
				}
			}

			addr2 = addr;
			while( ((c=API_I2C1_u8Get(addr))>=32) && (addr>I2C_REG_TB_U16_TERMINALBUFFER) )
			{
				addr--;
			}
			addr1 = addr+1;
			char tmpstr[(UI_W/CHAR_WIDTH)+1] = {};
			for(x=0;x<(UI_W/CHAR_WIDTH);x++)
			{
				if( x<(addr2-addr1) )
				{
					tmpstr[x] = API_I2C1_u8Get(addr1+x);
				}
				else
				{
					//tmpstr[x] = ' ';
					break;
				}
				tmpstr[x+1] = 0;
			}
			ILI9341_Draw_Text(tmpstr, 0, y, UI_FG_COLOR_1, 1, UI_BG_COLOR);
			if( x<(UI_W/CHAR_WIDTH) )
			{
				int w = (UI_W/CHAR_WIDTH) - x;
				ILI9341_Draw_Rectangle(x*CHAR_WIDTH,y,w*CHAR_WIDTH,CHAR_HEIGHT,UI_BG_COLOR);
			}

			y -= CHAR_HEIGHT;
		} while( y > (UI_H - 18*CHAR_HEIGHT) );
	}
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

#if 0
		static int test = 0;
		int i;
		for(int i=0;i<test;i++)
		{
			API_I2C1_u8Set(I2C_REG_TB_U16_TERMINALBUFFER,'a' + (i%26));
		}
		API_I2C1_u8Set(I2C_REG_TB_U16_TERMINALBUFFER,10);
		API_I2C1_u8Set(I2C_REG_TB_U16_TERMINALBUFFER,13);
		test++;
		if(test >= 100) test = 0;
#endif

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
		if( API_I2C1_u8Get(I2C_REG_TB_U8_PWRCNTDWN) != 0 )
		{
			sprintf(tmpstr,"%02d",API_I2C1_u8Get(I2C_REG_TB_U8_PWRCNTDWN));
		}
		else
		{
			sprintf(tmpstr,"  ",API_I2C1_u8Get(I2C_REG_TB_U8_PWRCNTDWN));
		}
		ILI9341_Draw_Text(tmpstr, UI_W-2*UI_CHAR_WIDTH, 0*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);
		ILI9341_Draw_Text("PWR:", UI_W-4*UI_CHAR_WIDTH, 1*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);

		uint8_t m = API_I2C1_u8Get(I2C_REG_TB_U8_PWRMODE_NEXT);
		if( m == PWRMODE_UNDEF )
		{
			m = API_I2C1_u8Get(I2C_REG_TB_U8_PWRMODE);
		}
		if( m == PWRMODE_OFF )
		{
			ILI9341_Draw_Text(" OFF", UI_W-4*UI_CHAR_WIDTH, 2*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);
		}
		else if(m == PWRMODE_ON )
		{
			ILI9341_Draw_Text("  ON", UI_W-4*UI_CHAR_WIDTH, 2*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);
		}
		else if( m == PWRMODE_AUTO )
		{
			ILI9341_Draw_Text("AUTO", UI_W-4*UI_CHAR_WIDTH, 2*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);
		}
		else
		{
			ILI9341_Draw_Text("----", UI_W-4*UI_CHAR_WIDTH, 2*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);
		}
	}

	{
		int r = 3;
		int u;

		u = API_I2C1_u16Get(I2C_REG_TB_U16_UBAT_MV);
		sprintf(tmpstr,"UBAT:%4d",(int)u);
		if( u > UBAT_FULL )
		{
			ILI9341_Draw_Text(tmpstr, 0, 1+(r++)*UI_CHAR_HEIGHT, GREEN, UI_CHAR_SIZE, UI_BG_COLOR);
		}
		else if( u > UBAT_MID )
		{
			ILI9341_Draw_Text(tmpstr, 0, 1+(r++)*UI_CHAR_HEIGHT, CYAN, UI_CHAR_SIZE, UI_BG_COLOR);
		}
		else if( u > UBAT_MIN )
		{
			ILI9341_Draw_Text(tmpstr, 0, 1+(r++)*UI_CHAR_HEIGHT, YELLOW, UI_CHAR_SIZE, UI_BG_COLOR);
		}
		else
		{
			ILI9341_Draw_Text(tmpstr, 0, 1+(r++)*UI_CHAR_HEIGHT, RED, UI_CHAR_SIZE, UI_BG_COLOR);
		}

		u = API_I2C1_u16Get(I2C_REG_TB_U16_UCHARGE_MV);
		sprintf(tmpstr,"UCH:%4d",u);
		ILI9341_Draw_Text(tmpstr, 0, 1+(r++)*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);

		u = API_I2C1_u16Get(I2C_REG_TB_U16_TEMP_C);
		sprintf(tmpstr,"Temp:%2d",u);
		ILI9341_Draw_Text(tmpstr, 0, 1+(r++)*UI_CHAR_HEIGHT, UI_FG_COLOR_1, UI_CHAR_SIZE, UI_BG_COLOR);
	}

	ui_update_terminal();
}


void ui_init()
{
	ILI9341_Init();	//initial driver setup to drive ili9341
	ILI9341_Fill_Screen(UI_BG_COLOR);
	ILI9341_Set_Rotation(SCREEN_HORIZONTAL_2);

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

