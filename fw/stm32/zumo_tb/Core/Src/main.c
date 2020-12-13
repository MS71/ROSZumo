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

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void setPWM(TIM_HandleTypeDef* timer, uint32_t channel, uint16_t period, uint16_t pulse)
{
	HAL_TIM_PWM_Stop(timer, channel);    // stop generation of pwm
	TIM_OC_InitTypeDef sConfigOC;
	timer->Init.Period = period;           // set the period duration
	HAL_TIM_PWM_Init(timer);  // reinititialise with new period value
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = pulse;              // set the pulse duration
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	HAL_TIM_PWM_ConfigChannel(timer, &sConfigOC, channel);
	HAL_TIM_PWM_Start(timer, channel);   // start pwm generation}
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

  //HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
  //setPWM(&htim16, TIM_CHANNEL_1, 10000, 7000);


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

void HAL_I2C_MemWriteCB(uint8_t addr)
{
	if( addr >= I2C_REG_TB_U16_VL53L1X_RSTREG && addr <= (I2C_REG_TB_U16_VL53L1X_RSTREG+1) )
	{
		vUpdateRSTGPIOs();
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
