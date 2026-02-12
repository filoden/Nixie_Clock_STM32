/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "app_fatfs.h"
#include "i2c.h"
#include "i2s.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <stdio.h> // used for DEBUG only
#include <string.h> // used for DEBUG UART handling
#include "debugging.h"
#include "cap1206.h"
#include "stm32g0xx_hal.h"
#include "UserInterface.h"
#include "SD_test.h"
#include "audio.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define ROT_TIMEOUT 500	 // Switch timeout feature used to regulate rotary knob impulses (ms)
#define ROT_TIMEIN 5 // switch debounce time (ms)
#define ROT_COOLDOWN 10
#define SW_DEB 100 // time in ms used to debounce pushbutton
#define START_TIME 120000
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint32_t FAILURE_CODES = 0; // songs.txt open failure |

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */



typedef struct {
	uint32_t A1;
	uint32_t A2;
	uint32_t B1;
	uint32_t B2;
	uint32_t C1;
	uint32_t C2;
	uint32_t SW;
	uint8_t prev_field: 8;			//[ x | x | C2 | C1 | B2 | B1 | A2 | A1 ]
} InterruptTable;

volatile InterruptTable intDataR;
volatile InterruptTable intDataF;
volatile int16_t interrupt_count = 0;


void interrupt_initializer(){
	intDataR.A1 = 0;
	intDataR.A2 = 0;
	intDataR.B1 = 0;
	intDataR.B2 = 0;
	intDataR.C1 = 0;
	intDataR.C2 = 0;
	intDataR.SW = 0;
	intDataR.prev_field = 0;			//[ x | x | C2 | C1 | B2 | B1 | A2 | A1 ]
	intDataF.A1 = 0;
	intDataF.A2 = 0;
	intDataF.B1 = 0;
	intDataF.B2 = 0;
	intDataF.C1 = 0;
	intDataF.C2 = 0;
	intDataF.SW = 0;
	intDataF.prev_field = 0;			//[ x | x | C2 | C1 | B2 | B1 | A2 | A1 ]
	return;
}




void clock_init(){
	uint32_t start_time = START_TIME;
	start_time++;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  interrupt_initializer();
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C2_Init();
  MX_I2S2_Init();
  MX_RTC_Init();
  MX_TIM1_Init();
  if (MX_FATFS_Init() != APP_OK) {
    Error_Handler();
  }
  MX_USART2_UART_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  // expect exti9_port == 0 for PA9
  //SD_Card_Test();
  printstr("hello!\n");
  FATFS fs;
  fileData fn = {0};
  SD_init(&fs, &fn);
  blockingPlaySound(&fn, 0);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  //debugnote("hey guysssss");

  //f_mount(&fs, "0:", 1);

  //parsewavheader(&fil, &fmt1);

  while(1){}
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	 //d1();
	 user_interaction_mode();
	 HAL_Delay(250);

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
}

/* USER CODE BEGIN 4 */
/* USER CODE BEGIN 4 */


// Rotary encoder groups with direction parity: PA0 / PA3, PB1 / PA9, PA10 / PB5
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin){
	interrupt_count++;
	uint32_t t_ms;
	t_ms = HAL_GetTick();
	int32_t t_elapsed_ms = 0;
	static volatile uint8_t *tail = NULL;
	if (USER_INT_REG7 == 0b0){
		tail = &USER_INT_REG7;
	}
	else if (USER_INT_REG6 == 0b0){
		tail = &USER_INT_REG6;
	}
	else if (USER_INT_REG5 == 0b0){
		tail = &USER_INT_REG5;
	}
	else if (USER_INT_REG4 == 0b0){
		tail = &USER_INT_REG4;
	}
	else if (USER_INT_REG3 == 0b0){
		tail = &USER_INT_REG3;
	}
	else if (USER_INT_REG2 == 0b0){
		tail = &USER_INT_REG2;
	}
	else if (USER_INT_REG1 == 0b0){
		tail = &USER_INT_REG1;
	}
	else if (USER_INT_REG0 == 0b0){
		tail = &USER_INT_REG0;
	}
	else {
		FAILURE_CODE = 1;
		return;
	}


	switch(GPIO_Pin){
		case GPIO_PIN_0 :  //A0
			//d2();
			intDataR.A1 = t_ms;
			intDataF.A1 = 0;
			t_elapsed_ms = intDataR.A1 - intDataR.A2;
			*tail = 0b00000001;
			break;
		case GPIO_PIN_3 : //D3
			//d3();
			intDataR.A2 = t_ms;
			intDataF.A2 = 0;
			t_elapsed_ms = intDataR.A2 - intDataR.A1;
			*tail = 0b00000010;
			break;
		case GPIO_PIN_1 : //B1
			intDataR.B1 = t_ms;
			intDataF.B1 = 0;
			t_elapsed_ms = intDataR.B1 - intDataR.B2;
			*tail = 0b00000100;
			break;
		case GPIO_PIN_9 : // A9
			intDataR.B2 = t_ms;
			intDataF.B2 = 0;
			t_elapsed_ms = intDataR.B2 - intDataR.B1;
			*tail = 0b00001000;
			break;
		case GPIO_PIN_10 : //A10
			intDataR.C1 = t_ms;
			intDataF.C1 = 0;
			t_elapsed_ms = intDataR.C1 - intDataR.C2;
			*tail = 0b00010000;
			break;
		case GPIO_PIN_5 : // B5
			intDataR.C2 = t_ms;
			intDataF.C2 = 0;
			t_elapsed_ms = intDataR.C2 - intDataR.C1;
			*tail = 0b00100000;
			break;
		case GPIO_PIN_6 :
			uint32_t t_elapsed_sw_msF = 0;
			uint32_t t_elapsed_sw_msR = 0;
			t_elapsed_sw_msF = t_ms - intDataF.SW;
			t_elapsed_sw_msR = t_ms - intDataR.SW;
			intDataR.SW = t_ms;
			*tail = 0b01000000;
			if ((t_elapsed_sw_msR > SW_DEB) && (t_elapsed_sw_msF > SW_DEB)  ){
				*tail = (*tail | 0b10000000);
				return;
				//printstr("switch above deb threshold");
			}
			else{
				*tail = 0;
				return;
					//printstr("interrupt failed timing thresholds");
			}
			break;
		default :
			FAILURE_CODE = 2;
			break;
	}
	if ((t_elapsed_ms < ROT_TIMEOUT) && (t_elapsed_ms > ROT_TIMEIN) && ((t_ms - LAST_ROTATION) > ROT_COOLDOWN) ){
		//intDataR.A1 = 0; 			// This and the line below prevents erroneous switching when switching is fast.
		//intDataR.A2 = 0;			//
		*tail = (*tail | 0b10000000);
		LAST_ROTATION = t_ms;
		//printstr("ROTARY TIME PASS");
	}
	else{
		*tail = 0;
		//printstr("ROTARY TIME FAIL");
	}
	return;
}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin){
	interrupt_count++;
	uint32_t t_ms;
	t_ms = HAL_GetTick();
	int32_t t_elapsed_ms = 0;

	static volatile uint8_t *tail = NULL;
	if (USER_INT_REG7 == 0b0){
		tail = &USER_INT_REG7;
	}
	else if (USER_INT_REG6 == 0b0){
		tail = &USER_INT_REG6;
	}
	else if (USER_INT_REG5 == 0b0){
		tail = &USER_INT_REG5;
	}
	else if (USER_INT_REG4 == 0b0){
		tail = &USER_INT_REG4;
	}
	else if (USER_INT_REG3 == 0b0){
		tail = &USER_INT_REG3;
	}
	else if (USER_INT_REG2 == 0b0){
		tail = &USER_INT_REG2;
	}
	else if (USER_INT_REG1 == 0b0){
		tail = &USER_INT_REG1;
	}
	else if (USER_INT_REG0 == 0b0){
		tail = &USER_INT_REG0;
	}
	else {
		FAILURE_CODE = 1;
		return;
	}

	switch(GPIO_Pin){
		case GPIO_PIN_0 :
			intDataF.A1 = t_ms;
			intDataR.A1 = 0;
			t_elapsed_ms = intDataF.A1 - intDataF.A2;
			*tail = 0b00000001;
			break;
		case GPIO_PIN_3 :
			intDataF.A2 = t_ms;
			intDataR.A2 = 0;
			t_elapsed_ms = intDataF.A2 - intDataF.A1;
			*tail = 0b00000010;
			break;
		case GPIO_PIN_1 :
			intDataF.B1 = t_ms;
			intDataR.B1 = 0;
			t_elapsed_ms = intDataF.B1 - intDataF.B2;
			*tail = 0b00000100;
			break;
		case GPIO_PIN_9 :
			intDataF.B2 = t_ms;
			intDataR.B2 = 0;
			t_elapsed_ms = intDataF.B2 - intDataF.B1;
			*tail = 0b00001000;
			break;
		case GPIO_PIN_10 :
			intDataF.C1 = t_ms;
			intDataR.C1 = 0;
			t_elapsed_ms = intDataF.C1 - intDataF.C2;
			*tail = 0b00010000;
			break;
		case GPIO_PIN_5 :
			intDataF.C2 = t_ms;
			intDataR.C2 = 0;
			t_elapsed_ms = intDataF.C2 - intDataF.C1;
			*tail = 0b00100000;
			break;
		case GPIO_PIN_6 :
			uint32_t t_elapsed_sw_ms = 0;
			t_elapsed_sw_ms = t_ms - intDataR.SW;
			intDataR.SW = t_ms;
			if (t_elapsed_sw_ms > SW_DEB){
				return;
				//printstr("switch above deb threshold");
			}
			else{
				*tail = 0;
				return;
					//printstr("interrupt failed timing thresholds");
			}
			break;
		default :
			FAILURE_CODE = 2;
			break;
	}
	if ((t_elapsed_ms < ROT_TIMEOUT) && (t_elapsed_ms > ROT_TIMEIN) && ((t_ms - LAST_ROTATION) > ROT_COOLDOWN) ){
		//intDataF.A1 = 0;
		//intDataF.A2 = 0;
		*tail = (*tail | 0b10000000);
		LAST_ROTATION = t_ms;
		//printstr("rotation Time within valid range");
	}
	else{
		*tail = 0;
		//printstr("interrupt failed timing thresholds");
	}
	return;
}
void HAL_SYSTICK_Callback(void)
{
  disk_timerproc();
  return;
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
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
