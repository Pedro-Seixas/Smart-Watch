/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#define LSM6DS3_ADDR  (0x6B << 1)
#define CTRL1_XL             0x10

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
 RTC_TimeTypeDef sTime;
 RTC_DateTypeDef sDate;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
 typedef enum{
	  DISPLAY_CLOCK,
	  CHANGE_TIME,
	  DISPLAY_SENSORS,
	  SHOW_MENU
 } menu;

 // Flags for the menu
 volatile menu main_menu;
 volatile uint8_t menu_active = 0;
 volatile uint8_t select_pressed = 0;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

RTC_HandleTypeDef hrtc;

/* Definitions for menu */
osThreadId_t menuHandle;
const osThreadAttr_t menu_attributes = {
  .name = "menu",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for imu */
osThreadId_t imuHandle;
const osThreadAttr_t imu_attributes = {
  .name = "imu",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
static void MX_RTC_Init(void);
void Menu_Task(void *argument);
void Imu_Task(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void LSM6DS3_Init(I2C_HandleTypeDef *hi2c) {
    uint8_t ctrl1_xl = 0x40;
    uint8_t ctrl2_g = 0x40;

    HAL_I2C_Mem_Write(hi2c, LSM6DS3_ADDR, CTRL1_XL, I2C_MEMADD_SIZE_8BIT, &ctrl1_xl, 1, HAL_MAX_DELAY);
    HAL_I2C_Mem_Write(hi2c, LSM6DS3_ADDR, 0x11, I2C_MEMADD_SIZE_8BIT, &ctrl2_g, 1, HAL_MAX_DELAY);
}

void LSM6DS3_WriteRegister(uint8_t reg, uint8_t value) {
    HAL_I2C_Mem_Write(&hi2c1, LSM6DS3_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, HAL_MAX_DELAY);
}

void LSM6DS3_ReadAccel(I2C_HandleTypeDef *hi2c, int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t accelData[6];
	uint16_t OUTX_L_XL = 0x28;

    // Read 6 bytes from OUTX_L_XL to OUTZ_H_XL (Accelerometer)
    HAL_I2C_Mem_Read(hi2c, LSM6DS3_ADDR, OUTX_L_XL, I2C_MEMADD_SIZE_8BIT, accelData, 6, HAL_MAX_DELAY);

    // Combine high and low bytes
    *ax = (int16_t)(accelData[1] << 8 | accelData[0]);
    *ay = (int16_t)(accelData[3] << 8 | accelData[2]);
    *az = (int16_t)(accelData[5] << 8 | accelData[4]);
}

void LSM6DS3_ReadGyro(I2C_HandleTypeDef *hi2c, int16_t *gx, int16_t *gy, int16_t *gz) {
    uint8_t gyroData[6];
    uint16_t OUTX_L_G = 0x22;

    // Read 6 bytes from OUTX_L_G (Gyroscope)
    HAL_I2C_Mem_Read(hi2c, LSM6DS3_ADDR, OUTX_L_G, I2C_MEMADD_SIZE_8BIT, gyroData, 6, HAL_MAX_DELAY);

    // Combine high and low bytes
    *gx = (int16_t)(gyroData[1] << 8 | gyroData[0]);
    *gy = (int16_t)(gyroData[3] << 8 | gyroData[2]);
    *gz = (int16_t)(gyroData[5] << 8 | gyroData[4]);
}

void LSM6DS3_ReadTemp(I2C_HandleTypeDef *hi2c, int16_t *temp){
	uint8_t tempData[2];
	uint16_t OUT_TEMP_L = 0x20;

    // Read 2 bytes from OUTX_TEMP_L (Temperature Sensor)
    HAL_I2C_Mem_Read(hi2c, LSM6DS3_ADDR, OUT_TEMP_L, I2C_MEMADD_SIZE_8BIT, tempData, 2, HAL_MAX_DELAY);

    // Combine high and low bytes
    *temp = (int16_t)(tempData[1] << 8 | tempData[0]);

}

void LSM6DS3_ReadStepsCount(I2C_HandleTypeDef *hi2c, uint16_t *steps){
	uint8_t stepsData[2];
	uint16_t STEP_COUNTER_L = 0x4B;

	// Read 2 bytes from STEP_COUNTER_L (Pedometer Sensor)
	HAL_I2C_Mem_Read(hi2c, LSM6DS3_ADDR, STEP_COUNTER_L, I2C_MEMADD_SIZE_8BIT, stepsData, 2, HAL_MAX_DELAY);

    // Combine high and low bytes
    *steps = (stepsData[1] << 8 | stepsData[0]);
}

void LSM6DS3_PedometerInit(I2C_HandleTypeDef *hi2c){
	uint8_t CTRL10_C = 0x19;

	// FUNC_EN bit 2 and PEDO_EN bit 4
	uint8_t ctrl10 = (1 << 2 ) | (1 << 4);

    HAL_I2C_Mem_Write(hi2c, LSM6DS3_ADDR, CTRL10_C, I2C_MEMADD_SIZE_8BIT, &ctrl10, 1, HAL_MAX_DELAY);
}

void LSM6DS3_WristTiltInit(I2C_HandleTypeDef *hi2c){
	uint8_t CTRL10_C = 0x19;

	// TILT_EN bit 3
	uint8_t ctrl10 = (1 << 2)| (1 << 7);

	HAL_I2C_Mem_Write(hi2c, LSM6DS3_ADDR, CTRL10_C, I2C_MEMADD_SIZE_8BIT, &ctrl10, 1, HAL_MAX_DELAY);

	// Configure a latency of 200ms
	uint8_t A_WRIST_TILT_LAT_C = 0x50;

	// Latency of 200ms for tilting
	uint8_t a_wrist_tilt_lat = 0x05;

	HAL_I2C_Mem_Write(hi2c, LSM6DS3_ADDR, A_WRIST_TILT_LAT_C, I2C_MEMADD_SIZE_8BIT, &a_wrist_tilt_lat, 1, HAL_MAX_DELAY);
}

uint8_t LSM6DS3_ReadWrist(I2C_HandleTypeDef *hi2c){
	uint8_t FUNC_SRC2 = 0x54;
	uint8_t wrist_tilt_detected;
	char msg[64];
	// Read Wrist Tilt
	HAL_I2C_Mem_Read(hi2c, LSM6DS3_ADDR, FUNC_SRC2, I2C_MEMADD_SIZE_8BIT, &wrist_tilt_detected, 1, HAL_MAX_DELAY);

	if (wrist_tilt_detected & 0x01) {
		int len = snprintf(msg, sizeof(msg), "Wrist Detected!\r\n");
		CDC_Transmit_FS((uint8_t*)msg, len);
	}

	return wrist_tilt_detected;
}

void SendAccelData(int16_t ax, int16_t ay, int16_t az) {
    char msg[64];
    int len = snprintf(msg, sizeof(msg), "AX: %d AY: %d AZ: %d\r\n", ax, ay, az);
    CDC_Transmit_FS((uint8_t*)msg, len);
}

void lsm6ds3tr_c_who_am_i(I2C_HandleTypeDef *hi2c) {
    uint8_t who_am_i = 0;
    HAL_I2C_Mem_Read(hi2c, LSM6DS3_ADDR, 0x0F, I2C_MEMADD_SIZE_8BIT, &who_am_i, 1, HAL_MAX_DELAY);

    char msg[32];
    int len = snprintf(msg, sizeof(msg), "WHO_AM_I: 0x%02X\r\n", who_am_i);
    CDC_Transmit_FS((uint8_t*)msg, len);
}

void menu_display_time(){
	HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

	char hour[100];
	char minute[100];

	ssd1306_Fill(Black);

	snprintf(hour, sizeof(hour), "%02d", sTime.Hours);
	snprintf(minute, sizeof(minute), "%02d", sTime.Minutes);

	// Show time (hour above minutes)
	ssd1306_SetCursor(16, 32);
	ssd1306_WriteString(hour, Font_16x26, White);
	ssd1306_SetCursor(16, 60);
	ssd1306_WriteString(minute, Font_16x26, White);
	ssd1306_UpdateScreen();
}

void show_menu(){
	menu static current_selected = DISPLAY_CLOCK;

	// Navigate menu options
	if(!HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4)){
		vTaskDelay(pdMS_TO_TICKS(200));
		main_menu = current_selected;
	}

	ssd1306_Fill(Black);

	// Show which option is selected
	switch(current_selected){
		case DISPLAY_CLOCK:
			ssd1306_SetCursor(2, 0);
			ssd1306_WriteString(">Clock", Font_7x10, White);
			ssd1306_SetCursor(2, 24);
			ssd1306_WriteString("Config", Font_7x10, White);
			ssd1306_SetCursor(2, 48);
			ssd1306_WriteString("Sensors", Font_7x10, White);
			break;
		case CHANGE_TIME:
			ssd1306_SetCursor(2, 0);
			ssd1306_WriteString("Clock", Font_7x10, White);
			ssd1306_SetCursor(2, 24);
			ssd1306_WriteString(">Config", Font_7x10, White);
			ssd1306_SetCursor(2, 48);
			ssd1306_WriteString("Sensors", Font_7x10, White);
			break;
		case DISPLAY_SENSORS:
			ssd1306_SetCursor(2, 0);
			ssd1306_WriteString("Clock", Font_7x10, White);
			ssd1306_SetCursor(2, 24);
			ssd1306_WriteString("Config", Font_7x10, White);
			ssd1306_SetCursor(2, 48);
			ssd1306_WriteString(">Sensors", Font_7x10, White);
			break;
		default:
			break;
	}

	ssd1306_UpdateScreen();

	if(menu_active && select_pressed){
		vTaskDelay(pdMS_TO_TICKS(200));
		current_selected = (current_selected + 1) % 3;
		select_pressed = 0;
	}
}

int8_t map_to_range(int16_t value) {
    // Map from int16_t to -90,90
    if (value >= 0)
        return (int8_t)(((int32_t)value * 90) / 32767);
    else
        return (int8_t)(((int32_t)value * 90) / 32768);
}

int convert_to_fahrenheit(int16_t rawTemp) {
    float tempC = 25.0f + ((float)rawTemp / 16.0f);
    float tempF = tempC * 1.8f + 32.0f;
    return (int)(tempC);
}

void show_sensors(){
	int16_t ax, ay, az, gx, gy, gz, tempInt;
	char gyro[50];
	char accel[50];
	char steps[50];
	uint16_t step;
	//char temp[50];

	// Read Sensors
	LSM6DS3_ReadGyro(&hi2c1, &gx, &gy, &gz);
	LSM6DS3_ReadAccel(&hi2c1, &ax, &ay, &az);
	LSM6DS3_ReadStepsCount(&hi2c1, &step);
	//LSM6DS3_ReadTemp(&hi2c1, &tempInt);

	// Display Sensors
	ssd1306_Fill(Black);

	// Convert to string
	snprintf(gyro, sizeof(gyro), "%d,%d,%d", map_to_range(gx), map_to_range(gy), map_to_range(gz));
	snprintf(accel, sizeof(accel), "%d,%d,%d", map_to_range(ax), map_to_range(ay), map_to_range(az));
	snprintf(steps, sizeof(steps), "%d", step);
	//snprintf(temp, sizeof(temp), "%d", convert_to_fahrenheit(tempInt));

	ssd1306_SetCursor(16, 0);
	ssd1306_WriteString("Gyro", Font_6x8, White);
	ssd1306_SetCursor(2, 12);
	ssd1306_WriteString(gyro, Font_6x8, White);

	ssd1306_SetCursor(16, 24);
	ssd1306_WriteString("Accel", Font_6x8, White);
	ssd1306_SetCursor(2, 36);
	ssd1306_WriteString(accel, Font_6x8, White);

	ssd1306_SetCursor(16, 48);
	ssd1306_WriteString("Steps", Font_6x8, White);
	ssd1306_SetCursor(2, 60);
	ssd1306_WriteString(steps, Font_6x8, White);

	// Removed Temperature as it is not reliable
	/*
	ssd1306_SetCursor(16, 48);
	ssd1306_WriteString("Temp", Font_6x8, White);
	ssd1306_SetCursor(16, 58);
	ssd1306_WriteString(temp, Font_6x8, White);
	 */

	ssd1306_UpdateScreen();
	vTaskDelay(pdMS_TO_TICKS(100));
}

void menu_change_time(){
    typedef enum{
        STATE_EDIT_HOUR,
        STATE_EDIT_MINUTE,
        STATE_CONFIRM_FIELD
    } MenuState;

    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    char hour[50];
    char minute[50];

    ssd1306_Fill(Black);

    static MenuState menu_selected = STATE_EDIT_HOUR;

    snprintf(hour, sizeof(hour), "%02d", sTime.Hours);
    snprintf(minute, sizeof(minute), "%02d", sTime.Minutes);

    // Select hour or minute
    if(menu_active && select_pressed){
        menu_selected = (menu_selected + 1) % 3;
        select_pressed = 0;
    }

    // Auto-repeat button variables
    static int button_was_pressed = 0;
    static uint32_t press_start_tick = 0;
    static uint32_t last_repeat_tick = 0;

    int button_pressed = !HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4);

    uint32_t current_tick = osKernelGetTickCount();

    // Auto-repeat timing constants
    const uint32_t initial_delay = 500 / portTICK_PERIOD_MS;
    const uint32_t repeat_interval = 100 / portTICK_PERIOD_MS;

    if(button_pressed){
        if(!button_was_pressed){
            // Button just pressed, increment once
            if(menu_selected == STATE_EDIT_HOUR){
                if(sTime.Hours < 23) sTime.Hours++;
                else sTime.Hours = 0;
                HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
            }
            else if(menu_selected == STATE_EDIT_MINUTE){
                if(sTime.Minutes < 59) sTime.Minutes++;
                else sTime.Minutes = 0;
                HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
            }
            else if(menu_selected == STATE_CONFIRM_FIELD){
                main_menu = SHOW_MENU;
            }
            press_start_tick = current_tick;
            last_repeat_tick = current_tick;
        } else {
            // Button held down, increment fast
            if((current_tick - press_start_tick) > initial_delay &&
               (current_tick - last_repeat_tick) > repeat_interval){
                // Repeat increment
                if(menu_selected == STATE_EDIT_HOUR){
                    if(sTime.Hours < 23) sTime.Hours++;
                    else sTime.Hours = 0;
                    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
                }
                else if(menu_selected == STATE_EDIT_MINUTE){
                    if(sTime.Minutes < 59) sTime.Minutes++;
                    else sTime.Minutes = 0;
                    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
                }

                last_repeat_tick = current_tick;
            }
        }
    } else {
        // Reset
        button_was_pressed = 0;
        press_start_tick = 0;
        last_repeat_tick = 0;
    }

    button_was_pressed = button_pressed;

    // Show cursors ( > ) based on the current state
    switch(menu_selected){
        case STATE_EDIT_HOUR:
            ssd1306_SetCursor(2, 32);
            ssd1306_WriteString(">", Font_11x18, White);
            break;
        case STATE_EDIT_MINUTE:
            ssd1306_SetCursor(2, 60);
            ssd1306_WriteString(">", Font_11x18, White);
            break;
        case STATE_CONFIRM_FIELD:
            ssd1306_SetCursor(2, 105);
            ssd1306_WriteString(">", Font_11x18, White);
            break;
    }

    // Show time (hour above minutes)
    ssd1306_SetCursor(16, 32);
    ssd1306_WriteString(hour, Font_16x26, White);
    ssd1306_SetCursor(16, 60);
    ssd1306_WriteString(minute, Font_16x26, White);
    ssd1306_SetCursor(16, 105);
    ssd1306_WriteString("Save", Font_11x18, White);
    ssd1306_UpdateScreen();
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	switch(GPIO_Pin){
		case GPIO_PIN_0:
			if (!menu_active) {
				menu_active = 1;
				main_menu = SHOW_MENU;
			}else {
				select_pressed = 1;
			}
			break;
		default:
			break;
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

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */

  // This is necessary for OLED Screen power-up
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, 0);
  HAL_Delay(1);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, 1);
  HAL_Delay(100);

  // Init accelerometer and gyroscope
  LSM6DS3_Init(&hi2c1);

  // Init Pedometer
  //LSM6DS3_PedometerInit(&hi2c1);

  // Init Wrist Tilt
  LSM6DS3_WristTiltInit(&hi2c1);

  // OLED Screen
  ssd1306_Init();
  ssd1306_Fill(Black);
  ssd1306_SetCursor(2, 0);
  ssd1306_WriteString("Hello", Font_11x18, White);
  ssd1306_SetCursor(2, 32);
  ssd1306_WriteString("World", Font_11x18, White);
  ssd1306_UpdateScreen();

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

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
  /* creation of menu */
  menuHandle = osThreadNew(Menu_Task, NULL, &menu_attributes);

  /* creation of imu */
  imuHandle = osThreadNew(Imu_Task, NULL, &imu_attributes);

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
  while (1)
  {
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE
                              |RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_12;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_3;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1|RCC_PERIPHCLK_RTC
                              |RCC_PERIPHCLK_USB;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
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
  hi2c1.Init.Timing = 0x00B07CB4;
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
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x0060112F;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

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

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 14;
  sTime.Minutes = 0;
  sTime.Seconds = 0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_WEDNESDAY;
  sDate.Month = RTC_MONTH_JULY;
  sDate.Date = 30;
  sDate.Year = 0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

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
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1|GPIO_PIN_2, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA1 PA2 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_1_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_Menu_Task */
/**
  * @brief  Function implementing the menu thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_Menu_Task */
void Menu_Task(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 5 */
  main_menu = DISPLAY_CLOCK;
  /* Infinite loop */
  for(;;)
  {
	  LSM6DS3_ReadWrist(&hi2c1);
	  switch(main_menu){
	  	  case DISPLAY_CLOCK:
	  		  menu_display_time();
	  		  menu_active = 0;
	  		  break;
	  	  case CHANGE_TIME:
	  	  	  menu_change_time();
	  		  break;
	  	  case DISPLAY_SENSORS:
	  		  menu_active = 0;
	  		  show_sensors();
	  		  break;
	  	  case SHOW_MENU:
	  		  menu_active = 1;
	  		  show_menu();
	  		  break;
	  	  default:
	  		  break;
	  }
	  vTaskDelay(pdMS_TO_TICKS(100));
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_Imu_Task */
/**
* @brief Function implementing the imu thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Imu_Task */
void Imu_Task(void *argument)
{
  /* USER CODE BEGIN Imu_Task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(100);
  }
  /* USER CODE END Imu_Task */
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
