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
#include "stm32f103xb.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_uart.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum {
  LED_MODE_ON,
  LED_MODE_OFF,
  LED_MODE_BLINK
} LED_Mode_t;

typedef void (*CommandHandler_t)(char *argument);

typedef struct {
  const char* name;
  CommandHandler_t handler;
  uint8_t require_argument;
} Command_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define BUTTON_DEBOUNCE_TIME 50
#define LED_PERIOD 500
#define HEARTBEAT_PERIOD 1000
#define RX_BUFFER_SIZE 32
#define UART_RX_RING_SIZE 64
#define COMMAND_BUFFER_SIZE 32
#define UART_TX_TIMEOUT 100

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

volatile uint8_t button_pressed = 0;
uint32_t last_button_tick = 0;

volatile uint32_t timer_tick  = 0;

uint8_t rx_data;
uint8_t rx_buffer[RX_BUFFER_SIZE];
uint8_t command_buffer[RX_BUFFER_SIZE];
uint8_t uart_rx_ring[UART_RX_RING_SIZE];
volatile uint8_t uart_rx_head = 0;
volatile uint8_t uart_rx_tail = 0;
volatile uint8_t rx_index = 0;
volatile uint8_t command_ready = 0;

LED_Mode_t led_mode = LED_MODE_BLINK;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);

/* USER CODE BEGIN PFP */
void LED_Task(void);
void Heartbeat_Task(void);
void Command_Task(void);

uint8_t UART_RingBuffer_Put(uint8_t data);
uint8_t UART_RingBuffer_Get(uint8_t data)
void UART_SendString(const char* str);
void Command_FindAndExecute(char *command, char *argument, char *extra_argument);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim2);
  HAL_UART_Receive_IT(&huart1, &rx_data, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    LED_Task();
    Heartbeat_Task();
    Command_Task();

    if (button_pressed) {
      button_pressed = 0;
      
      if(led_mode == LED_MODE_ON) {
        led_mode = LED_MODE_OFF;
      }
      else if (led_mode == LED_MODE_OFF) {
        led_mode = LED_MODE_ON;
      }
      else if (led_mode == LED_MODE_BLINK) {
        led_mode = LED_MODE_ON;
      }
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 7999;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 0;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_0) {
    uint32_t current_tick = HAL_GetTick();

    if((current_tick - last_button_tick) >= BUTTON_DEBOUNCE_TIME) {
      button_pressed = 1;
      last_button_tick = current_tick;
    }
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM2) {
    timer_tick++;
  }
}

void LED_Task(void) {
  static uint32_t last_tick = 0;
  static LED_Mode_t led_previous_mode = LED_MODE_OFF;

  if (led_mode != led_previous_mode) {
    led_previous_mode = led_mode;
    last_tick = timer_tick;
  }

  switch (led_mode) {
    case LED_MODE_ON:
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
      break;

    case LED_MODE_OFF:
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
      break;

    case LED_MODE_BLINK:
      if(timer_tick - last_tick >= LED_PERIOD) {
        last_tick += LED_PERIOD;
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
      }
      break;

    default:
      led_mode = LED_MODE_OFF;
      break;
  }
}

void Heartbeat_Task(void) {
  static uint32_t last_tick = 0;

  if((timer_tick - last_tick) >= HEARTBEAT_PERIOD) {
    last_tick += HEARTBEAT_PERIOD;
    // ...
  }
}

void Command_Task(void) {
  if (command_ready) {
    command_ready = 0;

    char* command;
    char* argument;
    char* extra_argument;

    command = strtok((char*)command_buffer, " ");
    argument = strtok(NULL, " ");
    extra_argument = strtok(NULL, " ");

    if (command == NULL) {
      UART_SendString("ERROR: UNKNOWN COMMAND\r\n");
      return;
    }

    if (strcmp(command, "LED") == 0) {
      if (extra_argument != NULL) {
        UART_SendString("ERROR: TOO MANY ARGUMENTS\r\n");
        return;
      }
    }
    else {
      if(argument != NULL) {
        UART_SendString("ERROR: TOO MANY ARGUMENTS\r\n");
        return;
      }
    }

    Command_FindAndExecute(command, argument, extra_argument);
  }
}

uint8_t UART_RingBuffer_Put(uint8_t data) {
  uint8_t next_head;
  next_head = (uart_rx_head + 1) % UART_RX_RING_SIZE;

  if (next_head == uart_rx_tail) {
    return 0;
  }

  uart_rx_ring[uart_rx_head] = data;
  uart_rx_head = next_head;

  return 1;
}

uint8_t UART_RingBuffer_Get(uint8_t *data) {
  uint8_t next_tail;

  if (uart_rx_tail == uart_rx_head) {
    return 0;
  }
  
  *data = uart_rx_ring[uart_rx_tail];

  next_tail = (uart_rx_tail + 1) % UART_RX_RING_SIZE;
  uart_rx_tail = next_tail;

  return 1;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    if (rx_data == '\n') {
      rx_buffer[rx_index] = '\0';

      if (!command_ready) {
        strcpy((char *)command_buffer, (char *)rx_buffer);
        command_ready = 1;
      }

      rx_index = 0;
    }
    else if (rx_data != '\r') {
      if (rx_index < RX_BUFFER_SIZE - 1) {
        rx_buffer[rx_index] = rx_data;
        rx_index++;
      }
    }
    HAL_UART_Receive_IT(&huart1, &rx_data, 1);
  }
}

void UART_SendString(const char* str) {
  HAL_UART_Transmit(
    &huart1, 
    (uint8_t *)str,
    strlen(str),
    UART_TX_TIMEOUT
  );
}

void LED_Handler(char *argument) {
  if (argument == NULL) {
    UART_SendString("ERROR: INVALID ARGUMENT\r\n");
    return;
  }
  if (strcmp(argument, "ON") == 0) {
    led_mode = LED_MODE_ON;

    UART_SendString("OK\r\n");
  }
  else if (strcmp(argument, "OFF") == 0) {
    led_mode = LED_MODE_OFF;

    UART_SendString("OK\r\n");
  }
  else if (strcmp(argument, "TOGGLE") == 0) {
    if (led_mode == LED_MODE_ON) {
      led_mode = LED_MODE_OFF;
    }
    // LED_MODE_OFF & LED_MODE_BLINK toggle to ON
    else {
      led_mode = LED_MODE_ON;
    }

    UART_SendString("OK\r\n");
  }
  else if (strcmp(argument, "BLINK") == 0) {
    led_mode = LED_MODE_BLINK;

    UART_SendString("OK\r\n");
  }
  else if (strcmp(argument, "STATUS") == 0) {
    if(led_mode == LED_MODE_ON) {
      UART_SendString("LED IS ON\r\n");
    }
    else if (led_mode == LED_MODE_OFF) {
      UART_SendString("LED IS OFF\r\n");
    }
    else {
      UART_SendString("LED IS BLINKING\r\n");
    }
  }
  else if (strcmp(argument, "RESET") == 0) {
    led_mode = LED_MODE_OFF;

    UART_SendString("OK: LED RESET\r\n");
  }
  else {
    UART_SendString("ERROR: INVALID ARGUMENT\r\n");
  }
}

void Help_Handler(char *argument) {
  (void)argument;

  UART_SendString(
    "Commands:\r\n"
        "LED ON\r\n"
        "LED OFF\r\n"
        "LED TOGGLE\r\n"
        "LED BLINK\r\n"
        "LED STATUS\r\n"
        "HELP\r\n"
        "VERSION\r\n"
  );
}

void Version_Handler(char* argument) {
  (void)argument;
  UART_SendString(
    "Firmware: STM32 LED Controller\r\n"
    "Version: 1.0\r\n"
  );
}

Command_t command_table[] = 
{
  {"LED", LED_Handler, 1},
  {"HELP", Help_Handler, 0},
  {"VERSION", Version_Handler, 0}
};

void Command_FindAndExecute(char *command, char *argument, char *extra_argument) {
  if (command == NULL) {
    UART_SendString("ERROR: UNKNOWN COMMAND\r\n");
    return;
  }

  size_t command_cnt = sizeof(command_table) / sizeof(command_table[0]);

  for(size_t i = 0; i < command_cnt; i++) {
    if (strcmp(command_table[i].name, command) == 0) {
      if (command_table[i].require_argument) {
        if (extra_argument != NULL) {
          UART_SendString("ERROR: TOO MANY ARGUMENTS\r\n");
          return;
        }
      }
      else {
        if (argument != NULL) {
          UART_SendString("ERROR: TOO MANY ARGUMENTS\r\n");
          return;
        }
      }
      command_table[i].handler(argument);
      return;
    }
  }

  UART_SendString("ERROR: UNKNOWN COMMAND\r\n");
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
