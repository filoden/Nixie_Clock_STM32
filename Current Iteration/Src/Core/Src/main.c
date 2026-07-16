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

#define ROT_TIMEOUT 3000	 // Switch timeout feature used to regulate rotary knob impulses (ms)
#define ROT_TIMEIN 0 // switch debounce time (ms)
#define ROT_COOLDOWN 0
#define SW_DEB 50 // time in ms used to debounce pushbutton
#define START_TIME 120000
#define OUTAGE_PIN_GROUP GPIOA
#define OUTAGE_PIN_NUM GPIO_PIN_4
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



typedef struct { // Used to measure the time of interrupt arrival in callback
	uint32_t A1;
	uint32_t A2;
	uint32_t B1;
	uint32_t B2;
	uint32_t C1;
	uint32_t C2;
	uint32_t SW;
} InterruptTable;

volatile InterruptTable intDataR; // R - Rising interrupt
volatile InterruptTable intDataF; // F - Falling interrupt
volatile int16_t interrupt_count = 0; // DEBUG


void interrupt_initializer(){ // Pre - intDataR and F are declared // Post - All values set to zero.
	intDataR.A1 = 0;
	intDataR.A2 = 0;
	intDataR.B1 = 0;
	intDataR.B2 = 0;
	intDataR.C1 = 0;
	intDataR.C2 = 0;
	intDataR.SW = 0;
	intDataF.A1 = 0;
	intDataF.A2 = 0;
	intDataF.B1 = 0;
	intDataF.B2 = 0;
	intDataF.C1 = 0;
	intDataF.C2 = 0;
	intDataF.SW = 0;
	USER_INT_REG0 = 0;
	USER_INT_REG1 = 0;
	USER_INT_REG2 = 0;
	USER_INT_REG3 = 0;
	USER_INT_REG4 = 0;
	USER_INT_REG5 = 0;
	USER_INT_REG6 = 0;
	USER_INT_REG7 = 0;
	LAST_ROTATION = 0;
	return;
}




void clock_init(){
	uint32_t start_time = START_TIME;
	start_time++;
}

#define NCH6100_SD_PORT GPIOA //NCH6100HV = 12V -> 170V DC -> DC converter
#define NCH6100_SD_PIN GPIO_PIN_1



void lowPowerMode(){
	printstr("\nEntering Low Power Mode.\n\r");
	HAL_GPIO_WritePin(NCH6100_SD_PORT, NCH6100_SD_PIN, GPIO_PIN_RESET);
	audio_mode = AUDIO_LOW_POWER; // set audio to play low power sound
	return;
}

void normalPowerMode(){
	printstr("Returning from LPM.\n\r");
			HAL_GPIO_WritePin(NCH6100_SD_PORT, NCH6100_SD_PIN, GPIO_PIN_SET);
			audio_mode = AUDIO_NORMAL; // set audio to play normally
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
  MX_USART2_UART_Init();
  MX_I2S2_Init();
  if (MX_FATFS_Init() != APP_OK) {
    Error_Handler();
  }
  MX_I2C2_Init();
  MX_RTC_Init();
  MX_SPI1_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  // expect exti9_port == 0 for PA9
  //SD_Card_Test();
  printstr("hello!\n");
  FATFS fs;
  HAL_Delay(100);
  SD_init(&fs, &fn);
  playSound(40);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  //debugnote("hey guysssss");

  //f_mount(&fs, "0:", 1);

  //parsewavheader(&fil, &fmt1);

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  // Wait for user input, delay 250ms to save power.
	  HAL_Delay(250);
	  if ((USER_INT_REG0 | USER_INT_REG1 | USER_INT_REG2 | USER_INT_REG3 | USER_INT_REG4 | USER_INT_REG5 | USER_INT_REG6 | USER_INT_REG7) != 0){
		  user_interaction_mode();
	  }
	  if (HAL_GPIO_ReadPin(OUTAGE_PIN_GROUP, OUTAGE_PIN_NUM) == GPIO_PIN_SET)
	  {
	     lowPowerMode();
	  }

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



void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin){
	uint32_t t_ms;
	t_ms = HAL_GetTick();
	uint32_t t_elapsed_ms = 0;
	static volatile uint8_t *tail = NULL;
	if (USER_INT_REG7 == 0b0){ // determines current tail of interrupt buffer (Each bit corresponds to a given interrupt)
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
		return;
	}


	switch(GPIO_Pin){
	// For any given interrupt (except PB):
	// 		Store time, set last falling interrupt time to zero.
	//		Determine time elapsed from (current time - time of last interrupt (of same type) occurring )
	// 		store interrupt pin in buffer

		case GPIO_PIN_0 :
			intDataR.A1 = t_ms;
			intDataF.A1 = 0;
			intDataF.A2 = 0;
			t_elapsed_ms = intDataR.A1 - intDataR.A2;
			*tail = 0b00000001;
			break;
		case GPIO_PIN_3 :
			intDataR.A2 = t_ms;
			intDataF.A1 = 0;
			intDataF.A2 = 0;
			t_elapsed_ms = intDataR.A2 - intDataR.A1;
			*tail = 0b00000010;
			break;
		case GPIO_PIN_1 :
			intDataR.B1 = t_ms;
			intDataF.B1 = 0;
			intDataF.B2 = 0;
			t_elapsed_ms = intDataR.B1 - intDataR.B2;
			*tail = 0b00000100;
			break;
		case GPIO_PIN_9 :
			intDataR.B2 = t_ms;
			intDataF.B1 = 0;
			intDataF.B2 = 0;
			t_elapsed_ms = intDataR.B2 - intDataR.B1;
			*tail = 0b00001000;
			break;
		case GPIO_PIN_10 :
			intDataR.C1 = t_ms;
			intDataF.C1 = 0;
			intDataF.C2 = 0;
			t_elapsed_ms = intDataR.C1 - intDataR.C2;
			*tail = 0b00010000;
			break;
		case GPIO_PIN_5 :
			intDataR.C2 = t_ms;
			intDataF.C1 = 0;
			intDataF.C2 = 0;
			t_elapsed_ms = intDataR.C2 - intDataR.C1;
			*tail = 0b00100000;
			break;
		case GPIO_PIN_6 :
		// The following is performed for the PB
		// 		determine time since last PB interrupt
		//		store time of current PB interrupt
		//		store interrupt pin in the buffer
		//		send PB signal if time since last rising AND falling PB interrupt is greater than the given threshold

			uint32_t t_elapsed_sw_msF = 0;
			uint32_t t_elapsed_sw_msR = 0;
			t_elapsed_sw_msF = t_ms - intDataF.SW;
			t_elapsed_sw_msR = t_ms - intDataR.SW;
			intDataR.SW = t_ms;
			*tail = 0b01000000;
			if ((t_elapsed_sw_msR > SW_DEB) && (t_elapsed_sw_msF > SW_DEB)  ){
				*tail = (*tail | 0b10000000);
				return;
			}
			else{
				*tail = 0;
				return;
			}
			break;
		default :
			break;
	}


	// Timing checks to reduce erroneous turning signals. Note - increasing ROT_TIMEOUT will result in undesirable behavior until the HAL_GetTick() surpases ROT_TIMEOUT. TIMEIN and COOLDOWN are currently set to zero.
	if ((t_elapsed_ms < ROT_TIMEOUT) && (t_elapsed_ms > ROT_TIMEIN) && ((t_ms - LAST_ROTATION) > ROT_COOLDOWN) ){
		*tail = (*tail | 0b10000000);
		LAST_ROTATION = t_ms;
	}
	else{
		*tail = 0;
	}
	return;
}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin){
	uint32_t t_ms;
	t_ms = HAL_GetTick();
	uint32_t t_elapsed_ms = 0;

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
		return;
	}

	switch(GPIO_Pin){
	// For any given interrupt (except PB):
	// 		Store time, set last falling interrupt time to zero.
	//		Determine time elapsed from (current time - time of last interrupt (of same type) occurring )
	// 		store interrupt pin in buffer

		case GPIO_PIN_0 :
			intDataF.A1 = t_ms;
			intDataR.A1 = 0;
			intDataR.A2 = 0;
			t_elapsed_ms = intDataF.A1 - intDataF.A2;
			*tail = 0b00000001;
			break;
		case GPIO_PIN_3 :
			intDataF.A2 = t_ms;
			intDataR.A1 = 0;
			intDataR.A2 = 0;
			t_elapsed_ms = intDataF.A2 - intDataF.A1;
			*tail = 0b00000010;
			break;
		case GPIO_PIN_1 :
			intDataF.B1 = t_ms;
			intDataR.B1 = 0;
			intDataR.B2 = 0;
			t_elapsed_ms = intDataF.B1 - intDataF.B2;
			*tail = 0b00000100;
			break;
		case GPIO_PIN_9 :
			intDataF.B2 = t_ms;
			intDataR.B1 = 0;
			intDataR.B2 = 0;
			t_elapsed_ms = intDataF.B2 - intDataF.B1;
			*tail = 0b00001000;
			break;
		case GPIO_PIN_10 :
			intDataF.C1 = t_ms;
			intDataR.C1 = 0;
			intDataR.C2 = 0;
			t_elapsed_ms = intDataF.C1 - intDataF.C2;
			*tail = 0b00010000;
			break;
		case GPIO_PIN_5 :
			intDataF.C2 = t_ms;
			intDataR.C1 = 0;
			intDataR.C2 = 0;
			t_elapsed_ms = intDataF.C2 - intDataF.C1;
			*tail = 0b00100000;
			break;
		case GPIO_PIN_6 :
			// The following is performed for the PB
			// 		determine time since last PB interrupt
			//		store time of current PB interrupt
			//		store interrupt pin in the buffer
			//		send PB signal if time since last rising AND falling PB interrupt is greater than the given threshold

			uint32_t t_elapsed_sw_ms = 0;
			t_elapsed_sw_ms = t_ms - intDataR.SW;
			intDataR.SW = t_ms;
			if (t_elapsed_sw_ms > SW_DEB){
				return;
			}
			else{
				*tail = 0;
				return;
			}
			break;
		default :
			break;
	}



	// Timing checks to reduce erroneous turning signals. Note - increasing ROT_TIMEOUT will result in undesirable behavior until the HAL_GetTick() surpases ROT_TIMEOUT. TIMEIN and COOLDOWN are currently set to zero.
	if ((t_elapsed_ms < ROT_TIMEOUT) && (t_elapsed_ms > ROT_TIMEIN) && ((t_ms - LAST_ROTATION) > ROT_COOLDOWN) ){
		*tail = (*tail | 0b10000000);
		LAST_ROTATION = t_ms;
	}
	else{
		*tail = 0;
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
