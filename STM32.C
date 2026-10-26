/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : DHT11 temperature and humidity on NUCLEO-F446RE
  *                   DATA pin = PA0 (A0). Output on PC COM port (USART2, 115200)
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DHT_PORT GPIOA
#define DHT_PIN  GPIO_PIN_0
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
static void delay_us_init(void);
static void delay_us(uint32_t us);
static uint8_t wait_level(GPIO_PinState level, uint32_t timeout_us);
static uint8_t dht11_read(uint8_t *humidity, uint8_t *temperature);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* Microsecond delay using the Cortex-M4 cycle counter (no timer needed) */
static void delay_us_init(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static void delay_us(uint32_t us)
{
  uint32_t start = DWT->CYCCNT;
  uint32_t ticks = us * (SystemCoreClock / 1000000U);
  while ((DWT->CYCCNT - start) < ticks) { }
}

/* Wait until the DATA pin reaches the given level. Returns 1 if OK, 0 on timeout */
static uint8_t wait_level(GPIO_PinState level, uint32_t timeout_us)
{
  uint32_t t = 0;
  while (HAL_GPIO_ReadPin(DHT_PORT, DHT_PIN) != level)
  {
    if (t++ >= timeout_us) return 0;
    delay_us(1);
  }
  return 1;
}

/* Read the DHT11. Returns 0 on success, non-zero error code otherwise */
static uint8_t dht11_read(uint8_t *humidity, uint8_t *temperature)
{
  uint8_t data[5] = {0, 0, 0, 0, 0};
  uint8_t result = 0;

  /* Start signal: pull DATA low for 20 ms, then release */
  HAL_GPIO_WritePin(DHT_PORT, DHT_PIN, GPIO_PIN_RESET);
  HAL_Delay(20);
  HAL_GPIO_WritePin(DHT_PORT, DHT_PIN, GPIO_PIN_SET);
  delay_us(30);

  __disable_irq();   /* timing critical part */

  if (!wait_level(GPIO_PIN_RESET, 100)) { result = 1; goto done; }   /* sensor pulls low  */
  if (!wait_level(GPIO_PIN_SET,   100)) { result = 2; goto done; }   /* sensor pulls high */
  if (!wait_level(GPIO_PIN_RESET, 100)) { result = 3; goto done; }   /* first bit starts  */

  for (int i = 0; i < 40; i++)
  {
    if (!wait_level(GPIO_PIN_SET, 100)) { result = 4; goto done; }   /* bit high starts */
    delay_us(40);
    if (HAL_GPIO_ReadPin(DHT_PORT, DHT_PIN) == GPIO_PIN_SET)
    {
      data[i / 8] |= (uint8_t)(1U << (7 - (i % 8)));                 /* still high = 1 */
    }
    if (!wait_level(GPIO_PIN_RESET, 100)) { result = 5; goto done; } /* bit ends */
  }

done:
  __enable_irq();

  if (result != 0) return result;

  if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) != data[4]) return 6;   /* checksum */

  *humidity    = data[0];
  *temperature = data[2];
  return 0;
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

  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  MX_GPIO_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  delay_us_init();

  char msg[80];
  uint8_t hum = 0, temp = 0;

  HAL_UART_Transmit(&huart2, (uint8_t*)"DHT11 reading started\r\n", 23, 200);
  HAL_Delay(1500);   /* DHT11 needs about 1 second after power up */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint8_t err = dht11_read(&hum, &temp);

    if (err == 0)
    {
      int n = snprintf(msg, sizeof(msg), "Temperature: %u C   Humidity: %u %%\r\n", temp, hum);
      HAL_UART_Transmit(&huart2, (uint8_t*)msg, n, 200);
      HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);   /* LD2 blinks on every good reading */
    }
    else
    {
      int n = snprintf(msg, sizeof(msg), "DHT11 error code: %u (check wiring)\r\n", err);
      HAL_UART_Transmit(&huart2, (uint8_t*)msg, n, 200);
    }

    HAL_Delay(2000);   /* DHT11 can be read once every 1 to 2 seconds */
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

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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
}

/**
  * @brief USART2 Initialization Function (PC via ST-Link, PA2 TX / PA3 RX)
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct2 = {0};
  __HAL_RCC_USART2_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  GPIO_InitStruct2.Pin = GPIO_PIN_2 | GPIO_PIN_3;
  GPIO_InitStruct2.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct2.Pull = GPIO_NOPULL;
  GPIO_InitStruct2.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct2.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct2);

  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* LD2 (PA5), starts OFF */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* DHT11 DATA on PA0: open-drain output with pull-up.
     Writing 1 releases the line, and the pin can still be read as an input. */
  HAL_GPIO_WritePin(DHT_PORT, DHT_PIN, GPIO_PIN_SET);
  GPIO_InitStruct.Pin = DHT_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(DHT_PORT, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{

}
#endif /* USE_FULL_ASSERT */
