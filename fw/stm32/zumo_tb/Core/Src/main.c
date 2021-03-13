/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "iwdg.h"
#include "lptim.h"
#include "usart.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "config.h"
#include "stm32g0xx_hal_pwr_ex.h"
#ifdef ENABLE_LCD_UI
#include "ui.h"
#endif
#include "pm.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

#define ENABLE_SEMI_PRINTF

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void FastSystemClock_Config(void);
uint8_t sysfastclock = 0;
extern void initialise_monitor_handles(void);
void vUpdateRSTGPIOs();
void vUpdateRPIGPIOs();

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* Buffer used for reception */
uint8_t ldx_rxbuf;
uint8_t lds_frmidx = 0;
uint8_t lds_frm[22];


/**
  * @brief  Rx Transfer completed callback
  * @param  UartHandle: UART handle
  * @note   This example shows a simple way to report end of DMA Rx transfer, and
  *         you can add your own implementation.
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	/*
	22 Bytes: <start> <index> <speed_L> <speed_H> [Data 0] [Data 1] [Data 2] [Data 3] <checksum_L> <checksum_H>

	start is always 0xFA
	index is the index byte in the 90 packets, going from 0xA0 (packet 0, readings 0 to 3) to 0xF9 (packet 89, readings 356 to 359).
	speed is a two-byte information, little-endian. It represents the speed, in 64th of RPM (aka value in RPM represented in fixed point, with 6 bits used for the decimal part).
	[Data 0] to [Data 3] are the 4 readings. Each one is 4 bytes long, and organized as follows :

	`byte 0 : <distance 7:0>`
	`byte 1 : <„invalid data“ flag> <„strength warning“ flag> <distance 13:8>`
	`byte 2 : <signal strength 7:0>`
	`byte 3 : <signal strength 15:8>`
	 */

	lds_frm[lds_frmidx] = ldx_rxbuf;
	if( lds_frm[0] == 0xFA )
	{
		lds_frmidx++;
	}
	if( lds_frmidx == 22 )
	{
		uint32_t chk32 = 0;
		int i;
		for(i=0; i<10; i++)
		{
		    chk32 = (chk32 << 1) + (lds_frm[2*i+0]|lds_frm[2*i+1]<<8);
		}
		chk32 = (chk32 & 0x7FFF) + (chk32 >> 15);
		chk32 &= 0x7FFF;
		if( ((chk32>>0)&0xff) == lds_frm[20] && ((chk32>>8)&0xff) == lds_frm[21] )
		{
			uint8_t index = lds_frm[1];
			double speed = (lds_frm[2] | lds_frm[3]<<8)/64.0;
			struct
			{
				uint8_t  distance;
				uint8_t  invalid;
				uint8_t  strength_warning;
				uint16_t signal;
			} data[4];
			if( speed < 300.0 )
			{
				if( htim16.Instance->CCR1 < 10000 )
					htim16.Instance->CCR1++;
			}
			else if( speed > 300.0 )
			{
				if( htim16.Instance->CCR1 > 1500 )
					htim16.Instance->CCR1--;
			}
			for(i=0;i<4;i++)
			{
				data[i].distance = (lds_frm[4+4*i+0] | (lds_frm[4+4*i+1]<<8))&0x3fff;
				data[i].invalid = (lds_frm[4+4*i+1]>>7)&1;
				data[i].strength_warning = (lds_frm[4+4*i+1]>>6)&1;
				data[i].signal = lds_frm[4+4*i+2] + lds_frm[4+4*i+3]<<8;
			}
			if( index == 0xA0 )
			{
				printf("0x%02x idx=%02x speed=%3.0f pwm=%d (%d,%d,%d,%d) (%d,%d,%d,%d) (%d,%d,%d,%d) (%d,%d,%d,%d) \n",
						lds_frm[0],
						index,
						speed,htim16.Instance->CCR1,
						data[0].distance,data[0].invalid,data[0].strength_warning,data[0].signal,
						data[1].distance,data[1].invalid,data[1].strength_warning,data[1].signal,
						data[2].distance,data[2].invalid,data[2].strength_warning,data[2].signal,
						data[3].distance,data[3].invalid,data[3].strength_warning,data[3].signal);
			}
		}
		lds_frm[0] = 0;
		lds_frmidx = 0;
	}

	if (HAL_UART_Receive_IT(&hlpuart1, (uint8_t *)&ldx_rxbuf, 1) != HAL_OK)
	{
		Error_Handler();
	}
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
#ifdef ENABLE_SEMI_PRINTF
  initialise_monitor_handles();
#endif
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_TIM1_Init();
  MX_RTC_Init();
  MX_ADC1_Init();
  MX_TIM16_Init();
  MX_LPTIM1_Init();
  MX_IWDG_Init();
  MX_I2C1_Init();
  MX_LPUART1_UART_Init();
  /* USER CODE BEGIN 2 */

  __HAL_RCC_PWR_CLK_ENABLE();

#if 0
  /* Disable Prefetch Buffer */
  __HAL_FLASH_PREFETCH_BUFFER_DISABLE();

  RCC->IOPSMENR  = 0x00u;
  RCC->AHBSMENR  = 0x00u;

  RCC->APBSMENR1 = 0x00u;
  RCC->APBSMENR2 = 0x00u;
#endif

  pm_early_init();

#ifdef HAL_IWDG_MODULE_ENABLED
	HAL_IWDG_Refresh(&hiwdg);
#endif

#if 0
  /* Enable Power Clock */
  __HAL_RCC_PWR_CLK_ENABLE();

  /* Reduce the System clock */
  SystemClock_Decrease();

  /* Set regulator voltage to scale 2 */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2);

  /* Enter LP RUN Mode */
  HAL_PWREx_EnableLowPowerRunMode();
#endif

  //printf("main() ...\n");

#ifdef ENABLE_LCD_UI
  ui_init();
#endif

  API_I2C1_u16Set(I2C_REG_TB_U16_RPI_DIR,0x0000);
  API_I2C1_u16Set(I2C_REG_TB_U16_RPI_WRITE,0x0000);
  API_I2C1_u16Set(I2C_REG_TB_U16_RPI_READ,0x0000);
  vUpdateRPIGPIOs();

  API_I2C1_Init();

  pm_init();

  /*
  HAL_GPIO_WritePin(O_L_RST_0_GPIO_Port,O_L_RST_0_Pin,GPIO_PIN_RESET);
  HAL_GPIO_WritePin(O_L_RST_1_GPIO_Port,O_L_RST_1_Pin,GPIO_PIN_RESET);
  HAL_GPIO_WritePin(O_L_RST_2_GPIO_Port,O_L_RST_2_Pin,GPIO_PIN_RESET);
  HAL_GPIO_WritePin(O_L_RST_3_GPIO_Port,O_L_RST_3_Pin,GPIO_PIN_RESET);
  HAL_GPIO_WritePin(O_L_RST_4_GPIO_Port,O_L_RST_4_Pin,GPIO_PIN_RESET);
  HAL_GPIO_WritePin(O_L_RST_5_GPIO_Port,O_L_RST_5_Pin,GPIO_PIN_RESET);
  HAL_GPIO_WritePin(O_L_RST_6_GPIO_Port,O_L_RST_6_Pin,GPIO_PIN_RESET);
  HAL_GPIO_WritePin(O_L_RST_7_GPIO_Port,O_L_RST_7_Pin,GPIO_PIN_RESET);
  HAL_GPIO_WritePin(O_L_RST_8_GPIO_Port,O_L_RST_8_Pin,GPIO_PIN_RESET);
  HAL_GPIO_WritePin(O_L_RST_9_GPIO_Port,O_L_RST_9_Pin,GPIO_PIN_RESET);
  HAL_GPIO_WritePin(O_L_RST_10_GPIO_Port,O_L_RST_10_Pin,GPIO_PIN_RESET);
  HAL_GPIO_WritePin(O_L_RST_11_GPIO_Port,O_L_RST_11_Pin,GPIO_PIN_RESET);
  HAL_GPIO_WritePin(O_BNO055_RESET_GPIO_Port,O_BNO055_RESET_Pin,GPIO_PIN_RESET);
*/

  API_I2C1_u16Set(I2C_REG_TB_U16_VL53L1X_RSTREG,0x0000);
  vUpdateRSTGPIOs();

  API_I2C1_u8Set(I2C_REG_TB_U8_WIFIFST,0x00);
  API_I2C1_u32Set(I2C_REG_TB_U32_IPADDR,0x00000000);

  HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);

#if 1
  if (HAL_UART_Receive_IT(&hlpuart1, (uint8_t *)&ldx_rxbuf, 1) != HAL_OK)
  {
    Error_Handler();
  }
#endif

#if 0
	HAL_Delay(500);

    /* Disable low power run mode and reset the clock to initialization configuration */
    HAL_PWREx_DisableLowPowerRunMode();

    /* Configure the system clock for the RUN mode */
    SystemClock_Config();
#endif

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	//printf("main() ...\n");
	while (1)
	{
#if 0
		{
			uint8_t buf;
			int n = 0;
			while( HAL_OK == HAL_UART_Receive(&hlpuart1, &buf, 1, 1 ) )
			{
				printf("%02x",buf);
				n++;
			}
			if(n > 0 )
			{
				printf("\n");
			}
		}
#endif

#if 0
		if( bRxBuffer_flag == 1 )
		{
			bRxBuffer_flag = 0;
			printf("%02x%02x%02x%02x%02x%02x%02x%02x\n",
					bRxBuffer[0],
					bRxBuffer[1],
					bRxBuffer[2],
					bRxBuffer[3],
					bRxBuffer[4],
					bRxBuffer[5],
					bRxBuffer[6],
					bRxBuffer[7]);
		}

#endif

#if 0
	{
	    GPIO_InitTypeDef GPIO_InitStruct = {0};
		GPIO_InitStruct.Pin = GPIO_PIN_10;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
		HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_10);
	}
#endif

#ifdef ENABLE_SEMI_PRINTF
#if 0
		printf("main() u_bat=%d u_charge=%d temp=%d charge=%d off=%d\n",
				API_I2C1_u16Get(I2C_REG_TB_U16_UBAT_MV),
				API_I2C1_u16Get(I2C_REG_TB_U16_UCHARGE_MV),
				API_I2C1_u16Get(I2C_REG_TB_U16_TEMP_C),
				HAL_GPIO_ReadPin(O_CHARGE_ON_GPIO_Port,O_CHARGE_ON_Pin),
				HAL_GPIO_ReadPin(ZUMO_SHDN_GPIO_Port,ZUMO_SHDN_Pin));
#endif
#endif // ENABLE_SEMI_PRINTF

		//HAL_WWDG_Refresh(WWDG);
		//HAL_IWDG_Refresh(IWDG);

		  //HAL_GPIO_TogglePin(O_LIDAR_M_GPIO_Port, O_LIDAR_M_Pin);

#if 0
	    HAL_GPIO_WritePin(ZUMO_SHDN_GPIO_Port,ZUMO_SHDN_Pin,GPIO_PIN_SET);
		while(1)
		{
		    HAL_GPIO_WritePin(O_CHARGE_ON_GPIO_Port,O_CHARGE_ON_Pin,GPIO_PIN_SET);
			HAL_Delay(1000);
		    HAL_GPIO_WritePin(O_CHARGE_ON_GPIO_Port,O_CHARGE_ON_Pin,GPIO_PIN_RESET);
			HAL_Delay(1000);
		}
#endif

#if 0
		HAL_Delay(5000);

		vStandby();
#endif

#if 0
		HAL_Delay(5000);

		HAL_GPIO_WritePin(O_LED2_GPIO_Port,O_LED2_Pin,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(ZUMO_SHDN_GPIO_Port,ZUMO_SHDN_Pin,GPIO_PIN_SET);

		HAL_SuspendTick();
		HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
		HAL_ResumeTick();

		/*
		 * ... Sleeping ...
		 */
		HAL_GPIO_WritePin(ZUMO_SHDN_GPIO_Port,ZUMO_SHDN_Pin,GPIO_PIN_SET);
		HAL_GPIO_WritePin(O_LED2_GPIO_Port,O_LED2_Pin,GPIO_PIN_SET);
		SystemClock_Config();
#endif

		API_I2C1_u32Set(I2C_REG_TB_U32_LOOP_CNT,API_I2C1_u32Get(I2C_REG_TB_U32_LOOP_CNT)+1);

		vUpdateRSTGPIOs();
		vUpdateRPIGPIOs();

		if( sysfastclock == 1 )
		{
			if( HAL_GPIO_ReadPin(ZUMO_SHDN_GPIO_Port,ZUMO_SHDN_Pin) == GPIO_PIN_SET )
			{
				SystemClock_Config();
				sysfastclock = 0;
			}
		}
		else
		{
			if( HAL_GPIO_ReadPin(ZUMO_SHDN_GPIO_Port,ZUMO_SHDN_Pin) == GPIO_PIN_RESET )
			{
				FastSystemClock_Config();
				sysfastclock = 1;
			}
		}

#ifdef ENABLE_LCD_UI
		ui_loop();
#endif
		pm_loop();

		API_I2C1_Handle();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 8;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV64;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the peripherals clocks
  */
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_LPUART1
                              |RCC_PERIPHCLK_LPTIM1|RCC_PERIPHCLK_I2C1
                              |RCC_PERIPHCLK_ADC|RCC_PERIPHCLK_TIM1;
  PeriphClkInit.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_HSI;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_SYSCLK;
  PeriphClkInit.Lptim1ClockSelection = RCC_LPTIM1CLKSOURCE_LSI;
  PeriphClkInit.AdcClockSelection = RCC_ADCCLKSOURCE_HSI;
  PeriphClkInit.Tim1ClockSelection = RCC_TIM1CLKSOURCE_PLL;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/**
  * @brief System Clock Configuration
  * @retval None
  */
void FastSystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 8;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the peripherals clocks
  */
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_LPTIM1
                              |RCC_PERIPHCLK_I2C1|RCC_PERIPHCLK_ADC
                              |RCC_PERIPHCLK_TIM1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_SYSCLK;
  PeriphClkInit.Lptim1ClockSelection = RCC_LPTIM1CLKSOURCE_LSI;
  PeriphClkInit.AdcClockSelection = RCC_ADCCLKSOURCE_HSI;
  PeriphClkInit.Tim1ClockSelection = RCC_TIM1CLKSOURCE_PLL;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

void vUpdateRSTGPIOs()
{
	if(API_I2C1_u8WRFlag(I2C_REG_TB_U16_VL53L1X_RSTREG,2)!=0)
	{
		int bit=0;
		uint16_t r = API_I2C1_u16Get(I2C_REG_TB_U16_VL53L1X_RSTREG);
		HAL_GPIO_WritePin(O_L_RST_0_GPIO_Port,O_L_RST_0_Pin,((r>>(bit++))&1)?GPIO_PIN_SET:GPIO_PIN_RESET);
		HAL_GPIO_WritePin(O_L_RST_1_GPIO_Port,O_L_RST_1_Pin,((r>>(bit++))&1)?GPIO_PIN_SET:GPIO_PIN_RESET);
		HAL_GPIO_WritePin(O_L_RST_2_GPIO_Port,O_L_RST_2_Pin,((r>>(bit++))&1)?GPIO_PIN_SET:GPIO_PIN_RESET);
		HAL_GPIO_WritePin(O_L_RST_3_GPIO_Port,O_L_RST_3_Pin,((r>>(bit++))&1)?GPIO_PIN_SET:GPIO_PIN_RESET);
		HAL_GPIO_WritePin(O_L_RST_4_GPIO_Port,O_L_RST_4_Pin,((r>>(bit++))&1)?GPIO_PIN_SET:GPIO_PIN_RESET);
		HAL_GPIO_WritePin(O_L_RST_5_GPIO_Port,O_L_RST_5_Pin,((r>>(bit++))&1)?GPIO_PIN_SET:GPIO_PIN_RESET);
		HAL_GPIO_WritePin(O_L_RST_6_GPIO_Port,O_L_RST_6_Pin,((r>>(bit++))&1)?GPIO_PIN_SET:GPIO_PIN_RESET);
		HAL_GPIO_WritePin(O_L_RST_7_GPIO_Port,O_L_RST_7_Pin,((r>>(bit++))&1)?GPIO_PIN_SET:GPIO_PIN_RESET);
		HAL_GPIO_WritePin(O_L_RST_8_GPIO_Port,O_L_RST_8_Pin,((r>>(bit++))&1)?GPIO_PIN_SET:GPIO_PIN_RESET);
		HAL_GPIO_WritePin(O_L_RST_9_GPIO_Port,O_L_RST_9_Pin,((r>>(bit++))&1)?GPIO_PIN_SET:GPIO_PIN_RESET);
		HAL_GPIO_WritePin(O_L_RST_10_GPIO_Port,O_L_RST_10_Pin,((r>>(bit++))&1)?GPIO_PIN_SET:GPIO_PIN_RESET);
		HAL_GPIO_WritePin(O_L_RST_11_GPIO_Port,O_L_RST_11_Pin,((r>>(bit++))&1)?GPIO_PIN_SET:GPIO_PIN_RESET);

		bit=15;
		HAL_GPIO_WritePin(O_BNO055_RESET_GPIO_Port,O_BNO055_RESET_Pin,((r>>(bit++))&1)?GPIO_PIN_SET:GPIO_PIN_RESET);
	}
}

void vUpdateRPIGPIOs()
{
	int i;
	const struct
	{
		GPIO_TypeDef*	port;
		uint16_t		pin;

	} rpi_pins[16] = {
			{RPI_11_GPIO_Port,RPI_11_Pin},
			{RPI_12_GPIO_Port,RPI_12_Pin},
			{RPI_13_GPIO_Port,RPI_13_Pin},
			{RPI_14_GPIO_Port,RPI_14_Pin},
			{RPI_15_GPIO_Port,RPI_15_Pin},
			{RPI_16_GPIO_Port,RPI_16_Pin},
			{RPI_18_GPIO_Port,RPI_18_Pin},
			{SPI_MOSI_GPIO_Port,SPI_MOSI_Pin},
			{SPI_MISO_GPIO_Port,SPI_MISO_Pin},
			{SPI_CS_GPIO_Port,SPI_CS_Pin},
			{SPI_CLK_GPIO_Port,SPI_CLK_Pin},
			{RPI_CS0_GPIO_Port,RPI_CS0_Pin},
			{RPI_CS1_GPIO_Port,RPI_CS1_Pin},
			{NULL,0xffff},
			{NULL,0xffff},
			{NULL,0xffff},
	};

	uint16_t reg_dir = API_I2C1_u16Get(I2C_REG_TB_U16_RPI_DIR);
	uint16_t reg_write = API_I2C1_u16Get(I2C_REG_TB_U16_RPI_WRITE);
	uint16_t reg_read = API_I2C1_u16Get(I2C_REG_TB_U16_RPI_READ);

	for( i=0; i<16 && rpi_pins[i].pin != 0xffff; i++)
	{
		uint8_t bit_dir = (reg_dir>>i)&1;
		uint8_t bit_write = (reg_write>>i)&1;

		if( bit_dir != 0 )
		{
			// should be an output

			  GPIO_InitTypeDef GPIO_InitStruct = {0};

			  GPIO_InitStruct.Pin = rpi_pins[i].pin;
			  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
			  GPIO_InitStruct.Pull = GPIO_NOPULL;
			  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
			  HAL_GPIO_Init(rpi_pins[i].port, &GPIO_InitStruct);

			  if( bit_write != 0 )
			  {
				  HAL_GPIO_WritePin(rpi_pins[i].port, rpi_pins[i].pin, GPIO_PIN_SET);
			  }
			  else
			  {
				  HAL_GPIO_WritePin(rpi_pins[i].port, rpi_pins[i].pin, GPIO_PIN_RESET);
			  }
		}
		reg_read &= ~(1<<i);
		reg_read |= (HAL_GPIO_ReadPin(rpi_pins[i].port,rpi_pins[i].pin)==GPIO_PIN_SET)?(1<<i):0;
	}
	API_I2C1_u16Set(I2C_REG_TB_U16_RPI_READ,reg_read);


}

void HAL_I2C_MemWriteCB(uint8_t addr)
{
	if( addr >= I2C_REG_TB_U16_VL53L1X_RSTREG && addr <= (I2C_REG_TB_U16_VL53L1X_RSTREG+1) )
	{
		vUpdateRSTGPIOs();
	}
	else if( addr >= I2C_REG_TB_U16_RPI_DIR && addr <= (I2C_REG_TB_U16_RPI_DIR+1) )
	{
		vUpdateRPIGPIOs();
	}
	else if( addr >= I2C_REG_TB_U16_RPI_READ && addr <= (I2C_REG_TB_U16_RPI_READ+1) )
	{
		vUpdateRPIGPIOs();
	}
	else if( addr >= I2C_REG_TB_U16_RPI_WRITE && addr <= (I2C_REG_TB_U16_RPI_WRITE+1) )
	{
		vUpdateRPIGPIOs();
	}
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	while(1)
	{
#ifdef O_LED2_Pin
		HAL_GPIO_WritePin(O_LED2_GPIO_Port,O_LED2_Pin,GPIO_PIN_SET);
#endif
		HAL_Delay(100);
#ifdef O_LED2_Pin
		HAL_GPIO_WritePin(O_LED2_GPIO_Port,O_LED2_Pin,GPIO_PIN_RESET);
#endif
		HAL_Delay(100);
	}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
