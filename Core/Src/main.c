/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : 倒车雷达系统 — 裸机版本
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "i2s.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "audio.h"
#include "oled.h"
#include "font.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
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
volatile uint8_t audio_play_finish = 1;
char message[20] = "";
char message1[20] = "";
char message2[20] = "";
int upEdge = 0;
int downEdge = 0;
float distance = 0;
uint8_t beep_state = 0;
uint32_t beep_tick = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void Audio_Play_DMA(const uint16_t *audio_buf, uint32_t len);
void Report_Distance(float dist);
void HCSR04_Trig(void);
void Beep_Control(void);
void OLED_Control(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HCSR04_Trig(void)
{
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
  for(uint32_t n=0; n<10; n++);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
  HAL_Delay(20);
  __HAL_TIM_SET_COUNTER(&htim1, 0);
}

void Audio_Play_DMA(const uint16_t *audio_buf, uint32_t len)
{
  while(audio_play_finish == 0);
  audio_play_finish = 0;
  HAL_I2S_Transmit_DMA(&hi2s2, (uint16_t*)audio_buf, len);
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
  if(hi2s->Instance == SPI2)
  {
    audio_play_finish = 1;
  }
}

void Report_Distance(float dist)
{
  if(audio_play_finish && distance < 200.0f)
  {
    Audio_Play_DMA(audio_data, audio_data_len);
  }
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if(htim == &htim1 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
  {
    upEdge = HAL_TIM_ReadCapturedValue(&htim1, TIM_CHANNEL_1);
    downEdge = HAL_TIM_ReadCapturedValue(&htim1, TIM_CHANNEL_2);
    distance = (downEdge - upEdge) * 0.034f / 2.0f;
  }
  sprintf(message, "距离:%.2f cm", distance);
}

void OLED_Control(void)
{
  OLED_NewFrame();
  OLED_PrintString(0, 0, message2, &font16x16, OLED_COLOR_NORMAL);
  OLED_PrintString(0, 18, message, &font16x16, OLED_COLOR_NORMAL);
  OLED_PrintString(0, 36, message1, &font16x16, OLED_COLOR_NORMAL);
  OLED_ShowFrame();
}

void Beep_Control(void)
{
  if(distance > 20.0f)
  {
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET);
    sprintf(message1, "提示:正常");
    beep_tick = HAL_GetTick();
  }
  else if(distance > 15.0f)
  {
    if(HAL_GetTick() - beep_tick >= 300)
    {
      beep_state = !beep_state;
      beep_tick = HAL_GetTick();
    }
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, beep_state ? GPIO_PIN_RESET : GPIO_PIN_SET);
    sprintf(message1, "提示:注意");
  }
  else if(distance > 10.0f)
  {
    if(HAL_GetTick() - beep_tick >= 200)
    {
      beep_state = !beep_state;
      beep_tick = HAL_GetTick();
    }
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, beep_state ? GPIO_PIN_RESET : GPIO_PIN_SET);
    sprintf(message1, "提示:危险");
  }
  else
  {
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
    sprintf(message1, "提示:停止");
  }
}
/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_I2C1_Init();
  MX_I2S2_Init();

  /* USER CODE BEGIN 2 */
  HAL_Delay(20);
  OLED_Init();

  HAL_TIM_Base_Start(&htim1);
  HAL_TIM_IC_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_2);
  /* USER CODE END 2 */

  while (1)
  {
    /* USER CODE BEGIN WHILE */
    sprintf(message2, "倒车请注意!!!");
    HCSR04_Trig();
    Report_Distance(distance);
    OLED_Control();
    Beep_Control();
    /* USER CODE END WHILE */
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2S2;
  PeriphClkInit.I2s2ClockSelection = RCC_I2S2CLKSOURCE_SYSCLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
