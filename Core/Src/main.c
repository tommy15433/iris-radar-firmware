/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
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
#include "cmsis_os.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "xensiv_radar_presence.h"

#include "xensiv_bgt60trxx_stm.h"
#include "xensiv_radar_presence.h"
#include "radar_config_optimizer.h"

#include "radar_config.h"
#define XENSIV_RADAR_PRESENCE_SETTINGS_H_IMPL
#include "presence_settings.h"

#include "optimization_list.h"

#include "user.h"
#include "arm_math.h"

#include "stdlib.h"
#include "usbd_cdc_if.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    xensiv_radar_presence_event_t last_reported_event;
    bool verbose;
    XENSIV_RADAR_PRESENCE_TIMESTAMP bookmark_timestamp;
}ce_state_s;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define PRINT_DATA 

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define PRINT_INFO(X) printf( X  "\r\n")
#define PRINT_NOTIFY(X) printf(ANSI_COLOR_GREEN X ANSI_COLOR_RESET "\r\n")
#define PRINT_ERR(X) printf(ANSI_COLOR_RED X ANSI_COLOR_RESET "\r\n")
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

I2C_HandleTypeDef hi2c1;

RTC_HandleTypeDef hrtc;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart6;

/* Definitions for dataManager */
osThreadId_t dataManagerHandle;
const osThreadAttr_t dataManager_attributes = {
  .name = "dataManager",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for dataProcessor */
osThreadId_t dataProcessorHandle;
const osThreadAttr_t dataProcessor_attributes = {
  .name = "dataProcessor",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for semManager */
osSemaphoreId_t semManagerHandle;
const osSemaphoreAttr_t semManager_attributes = {
  .name = "semManager"
};
/* Definitions for semProcessor */
osSemaphoreId_t semProcessorHandle;
const osSemaphoreAttr_t semProcessor_attributes = {
  .name = "semProcessor"
};
/* USER CODE BEGIN PV */

xensiv_radar_presence_handle_t handle;
ce_state_s ce_app_state;
static float32_t frame[NUM_SAMPLES_PER_FRAME * 2];
static float32_t avg_chirp[NUM_SAMPLES_PER_CHIRP];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_RTC_Init(void);
void dataManagerTask(void *argument);
void dataProcessorTask(void *argument);

/* USER CODE BEGIN PFP */
void reconf_radar(optimization_type_e requested);
void process_verbose_cmd(xensiv_radar_presence_handle_t handle, XENSIV_RADAR_PRESENCE_TIMESTAMP time_ms);
void presence_detection_cb(xensiv_radar_presence_handle_t handle, const xensiv_radar_presence_event_t* event, void *data);

size_t memory_usage = 0;
uint8_t data_buff[NUM_SAMPLES_PER_FRAME * 3];
uint16_t buff16[NUM_SAMPLES_PER_FRAME*2] = {0,};
uint32_t buff32[NUM_SAMPLES_PER_FRAME] = {0,};

void* custom_malloc(size_t size)
{
  memory_usage = memory_usage + size;
	printf("\r\nmemory in use: %x\r\n", memory_usage);
  return malloc(size); 
}
void custom_free(void* ptr)
{
	printf("custom free");
  free(ptr);
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

uint32_t getTick() {
  return xTaskGetTickCount();
}

int _write(int file, char* p, int len){
  CDC_Transmit_FS(p, len);
	return len;
}

void sendPacket(uint8_t detect, uint8_t distance)
{
  uint8_t packet[10];
  uint8_t chksum = 0;

  packet[0] = 0x55;
  packet[1] = 0xaa;
  packet[2] = 8;
  packet[3] = detect;
  packet[4] = 60+(getTick() % 10);
  packet[5] = 20+(getTick() % 10);
  packet[6] = distance;
  packet[7] = 0;
  packet[8] = 0;
  for (int i = 0; i < 9; i++)
    chksum ^= packet[i];
  packet[9] = chksum;
  HAL_UART_Transmit(&huart1, packet, 10, 10);
}

struct xensiv_bgt60trxx_type sensor_type = {
  .device = XENSIV_DEVICE_BGT60TR13C,
  .fifo_size = 8192U,
  .fifo_addr = 0x60U
};

xensiv_bgt60trxx_mtb_iface_t sensor_interface = {
  .irqpin = {
    .gpio = GPIOA,
    .pin = 1 << 0
  },
  .rstpin = {
    .gpio = GPIOA,
    .pin = 1 << 3
  },
  .selpin = {
    .gpio = GPIOA,
    .pin = 1 << 4
  },
  .spi = &hspi1
};

xensiv_bgt60trxx_mtb_t sensor_instance = {
  .dev = {
    .high_speed = true,
    .type = &sensor_type,
    .iface = (void*)&sensor_interface
  },
  .iface = {
		  .irqpin = {
		    .gpio = GPIOA,
		    .pin = 1 << 0
		  },
		  .rstpin = {
		    .gpio = GPIOA,
		    .pin = 1 << 3
		  },
		  .selpin = {
		    .gpio = GPIOA,
		    .pin = 1 << 4
		  },
		  .spi = &hspi1
  }
};

void initializations(void)
{
  // sensor init
	stm_result_t res = xensiv_bgt60trxx_mtb_init2(&sensor_instance, NUM_SAMPLES_PER_FRAME * 2, register_list, XENSIV_BGT60TRXX_CONF_NUM_REGS);
	printf("bgt init result: %x\n", res);
  
	/* Initialize the initial state of ce_app_state */
	ce_app_state.last_reported_event.state = XENSIV_RADAR_PRESENCE_STATE_ABSENCE;
	ce_app_state.last_reported_event.range_bin = 0;
	ce_app_state.last_reported_event.timestamp = 0;
  // ce_app_state.verbose = true;

  // main init

  xensiv_radar_presence_set_malloc_free(custom_malloc, custom_free);

  if (xensiv_radar_presence_alloc(&handle, &default_config) != 0)
  {
      CY_ASSERT(0);
  }

  xensiv_radar_presence_set_callback(handle, presence_detection_cb, NULL);
  int result = radar_config_optimizer_init(reconf_radar);

  if(result != ESTATUS_SUCCESS)
  {
      printf("[MSG] ERROR: radar_config_optimizer_init failed with error %" PRIi32 "\n", result);
      CY_ASSERT(0);
  }

  result = radar_config_optimizer_set_operational_mode(default_config.mode);

  if(result != ESTATUS_SUCCESS)
  {
      printf("[MSG] ERROR: radar_config_optimizer_set_operational_mode failed with error %" PRIi32 "\n", result);
      CY_ASSERT(0);
  }

  xensiv_bgt60trxx_set_fifo_limit(&sensor_instance.dev, NUM_SAMPLES_PER_FRAME);
	if (xensiv_bgt60trxx_start_frame(&sensor_instance.dev, true) != XENSIV_BGT60TRXX_STATUS_OK)
	{
    printf("start frame fail\r\n");
		CY_ASSERT(0);
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

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

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
  MX_SPI1_Init();
  MX_I2C1_Init();
  MX_USART6_UART_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */

  LED1_ON(); 
  LED2_ON(); 

  initializations();
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of semManager */
  semManagerHandle = osSemaphoreNew(1, 0, &semManager_attributes);

  /* creation of semProcessor */
  semProcessorHandle = osSemaphoreNew(1, 0, &semProcessor_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of dataManager */
  dataManagerHandle = osThreadNew(dataManagerTask, NULL, &dataManager_attributes);

  /* creation of dataProcessor */
  dataProcessorHandle = osThreadNew(dataProcessorTask, NULL, &dataProcessor_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

	while (1){
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

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 96;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x20303E5D;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  huart6.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart6.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3|GPIO_PIN_4, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10|GPIO_PIN_11, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA3 PA4 PA10 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB10 PB11 */
  GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/*******************************************************************************
 * Function Name: process_verbose_cmd
 ********************************************************************************
 * Summary:
 * This function processes the prints the desired output on CLI provided
 * user has set the "verbose enabled" through Cli task.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  none
 *
 *******************************************************************************/
void process_verbose_cmd(xensiv_radar_presence_handle_t handle,
        XENSIV_RADAR_PRESENCE_TIMESTAMP time_ms)
{
    float32_t energy = 0;
    int range_bin = 0;

    if (ce_app_state.verbose == false)
    {
        return;
    }

    if (ce_app_state.bookmark_timestamp + 1000 <= time_ms)
    {

        switch (ce_app_state.last_reported_event.state)
        {
            case XENSIV_RADAR_PRESENCE_STATE_MACRO_PRESENCE:
                LED1_ON();
                LED2_OFF();
                printf("[INFO] macro presence %" PRIi32 " %" PRIi32 "\r\n",
                        ce_app_state.last_reported_event.range_bin,
                        time_ms);
                break;

            case XENSIV_RADAR_PRESENCE_STATE_MICRO_PRESENCE:
                LED1_OFF();
                LED2_ON();
                printf("[INFO] micro presence %" PRIi32 " %" PRIi32 "\r\n",
                        ce_app_state.last_reported_event.range_bin,
                        time_ms);
                break;

            case XENSIV_RADAR_PRESENCE_STATE_ABSENCE:
                LED1_OFF();
                LED2_OFF();
                printf("[INFO] absence %" PRIu32 "\r\n", time_ms);
                break;

            default:
                printf("[MSG] ERROR: Unknown reported state in event handling\r\n");
                break;
        }

        const cfloat32_t *macro_fft_buff = xensiv_radar_presence_get_macro_fft_buffer(handle);

        printf("[MACRO_FFT] %lu",(unsigned long)time_ms);

        for(int i = 0; i< MACRO_FFT_BUFF_SIZE; i++)
        {
            float zero[2] = { 0.f, 0.f};

//            printf("%lf ", arm_euclidean_distance_f32((float*)&macro_fft_buff[i], zero, 2));
        }

        printf("\r\n");

        xensiv_radar_presence_get_max_macro(handle, &energy, &range_bin);
        printf("[MACRO] %d %lf %lu\r\n", range_bin, energy, (unsigned long)time_ms);

        xensiv_radar_presence_get_max_micro(handle, &energy, &range_bin);
        printf("[MICRO] %d %lf %lu\r\n", range_bin, energy, (unsigned long) time_ms);

        ce_app_state.bookmark_timestamp = time_ms;

    }
}

void presence_detection_cb(xensiv_radar_presence_handle_t handle,
                           const xensiv_radar_presence_event_t* event,
                           void *data)
{
    (void)handle;
    (void)data;

    static bool exist = false;

    // printf("\r\n\r\npresence detection callback!\r\n\r\n");
    if (!ce_app_state.verbose)
    {
        switch (event->state)
        {
            case XENSIV_RADAR_PRESENCE_STATE_MACRO_PRESENCE:
                LED1_ON();
                LED2_OFF();
                printf("[INFO] macro presence %" PRIi32 " %" PRIi32 "\n" ,
                        event->range_bin,
                        event->timestamp);
                
                // sendPacket(1,(uint8_t)(10*xensiv_radar_presence_get_bin_length(handle))*event->range_bin);

                        if (exist == false){
                          PRINT_INFO("Person has entered");
                          exist = true;
                        }
                break;

            case XENSIV_RADAR_PRESENCE_STATE_MICRO_PRESENCE:
                LED1_OFF();
                LED2_ON();
                printf("[INFO] micro presence %" PRIi32 " %" PRIi32 "\n",
                        event->range_bin,
                        event->timestamp);
                
                // sendPacket(1,(uint8_t)(10*xensiv_radar_presence_get_bin_length(handle))*event->range_bin);

                        if (exist == false){
                          PRINT_INFO("Person has entered");
                          exist = true;
                        }
                break;

            case XENSIV_RADAR_PRESENCE_STATE_ABSENCE:
                printf("[INFO] absence %" PRIu32 "\n", event->timestamp);
                LED1_OFF();
                LED2_OFF(); 

                // sendPacket(0,0);

                if (exist == true){
                  PRINT_ERR("Person has left");
                  exist = false;
                }
                break;

            default:
                printf("[MSG] ERROR: Unknown reported state in event handling\n");
                break;
        }

    }
    // quePush(que_cmd_presence_detection);
    /* save the last reported event state */
    float ff = xensiv_radar_presence_get_bin_length(handle) * default_config.max_range_bin;
    printf("range: %f\r\n", ff);
    ce_app_state.last_reported_event = *event;
}

/*********************************************************//**********************
* Function Name: reconf_radar
**************************************************//******************************
* Summary:
* This is the function for radar reconfiguration
*
* Parameters:
*  requested: Choosed configuration
*
* Return:
*  void
*
*******************************************************************************/
void reconf_radar(optimization_type_e requested)
{
  // return;

  // printf("reconfig radar\r\n");
  // return;

    if (requested == CONFIG_UNINITIALIZED)
    {
        return;
    }

    if (xensiv_bgt60trxx_config(&sensor_instance.dev,
            optimizations_list[requested].reg_list,
            optimizations_list[requested].reg_list_size) != 0)
    {
        printf("[MSG] ERROR: xensiv_bgt60trxx reconfiguration failed\n");
        CY_ASSERT(0);
    }

    if (xensiv_bgt60trxx_set_fifo_limit(&sensor_instance.dev,
            optimizations_list[requested].fifo_limit) != 0)
    {
        printf("[MSG] ERROR: xensiv_bgt60trxx set fifo limit failed\n");
        CY_ASSERT(0);
    }

    if (xensiv_bgt60trxx_start_frame(&sensor_instance.dev, true) != 0)
    {
        printf("[MSG] ERROR: xensiv_bgt60trxx_start_frame failed\n");
        CY_ASSERT(0);
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
  osSemaphoreRelease(semManagerHandle);
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_dataManagerTask */
/**
  * @brief  Function implementing the dataManager thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_dataManagerTask */
void dataManagerTask(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
   if (osSemaphoreAcquire(semManagerHandle, 0) == osOK) {
    if (xensiv_bgt60trxx_get_fifo_data(&sensor_instance.dev, (uint8_t*)data_buff, NUM_SAMPLES_PER_FRAME*3) != 0)
    {
      PRINT_ERR("FIFO READ FAILED");
      xensiv_bgt60trxx_soft_reset(&sensor_instance.dev, XENSIV_BGT60TRXX_RESET_FIFO);
      xensiv_bgt60trxx_start_frame(&sensor_instance.dev, true);
      continue;
    }

    xensiv_bgt60trxx_soft_reset(&sensor_instance.dev, XENSIV_BGT60TRXX_RESET_FIFO);
    xensiv_bgt60trxx_start_frame(&sensor_instance.dev, true);
    
    for (int i = 0; i < NUM_SAMPLES_PER_FRAME; i++) {
      buff32[i] = data_buff[i*3+0] << 16;
      buff32[i] |= data_buff[i*3+1] << 8;
      buff32[i] |= data_buff[i*3+2];

      buff16[i*2] = buff32[i]>>12;
      buff16[i*2+1] = buff32[i] & 0xfff;
    }

    #ifdef PRINT_DATA
    
    #endif
    osSemaphoreRelease(semProcessorHandle);

   }else{
    osDelay(1);
   }
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_dataProcessorTask */
/**
* @brief Function implementing the dataProcessor thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_dataProcessorTask */
void dataProcessorTask(void *argument)
{
  /* USER CODE BEGIN dataProcessorTask */

  /* Infinite loop */
  for(;;)
  {
    if (osSemaphoreAcquire(semProcessorHandle, 0) == osOK) {

      uint16_t *bgt60_buffer_ptr = buff16;//data_buff;
      float32_t *frame_ptr = &frame[0];
      for (int32_t sample = 0; sample < NUM_SAMPLES_PER_FRAME * 2; ++sample)
      {
        *frame_ptr++ = ((float32_t)(*bgt60_buffer_ptr++) / 4096.0F);
      }

      /* calculate the average of the chirps first */
      arm_fill_f32(0, avg_chirp, NUM_SAMPLES_PER_CHIRP);

      for (int chirp = 0; chirp < NUM_CHIRPS_PER_FRAME * 2; chirp++)
      {
        arm_add_f32(avg_chirp, &frame[NUM_SAMPLES_PER_CHIRP * chirp], avg_chirp, NUM_SAMPLES_PER_CHIRP);
      }

      arm_scale_f32(avg_chirp, 1.0f / NUM_CHIRPS_PER_FRAME, avg_chirp, NUM_SAMPLES_PER_CHIRP);

#ifdef PRINT_DATA
      // better raise uart baudrate to make print work @55
      for (int i = 0; i < NUM_SAMPLES_PER_CHIRP; i++) {
        printf("%.2f ", avg_chirp[i]);
      } 
      printf("\r\n");
#endif

      if (xensiv_radar_presence_process_frame(handle, avg_chirp, getTick()) != XENSIV_RADAR_PRESENCE_OK)
      {
        printf("process frame error \r\n");
      }

    } else {

      osDelay(1);
    }
  }
  /* USER CODE END dataProcessorTask */
}

/* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
