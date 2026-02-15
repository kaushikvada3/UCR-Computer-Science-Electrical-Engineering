/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : DIAGNOSTIC Main program body
  * Features: Auto-Detect Address (0x08/0x18) AND Auto-Detect CRC requirement
  * PLUS: 10-Channel STM32 ADC Thermistor Reading with Temperature Conversion
  * PLUS: Current Sensing from BQ76930 with 20mOhm Shunt
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SYS_STAT          0x00
#define SYS_CTRL1         0x04
#define SYS_CTRL2         0x05
#define VC1_HI_BYTE       0x0C
#define CC_HI_BYTE        0x32  // Current measurement high byte

// Current Shunt Parameters
#define SHUNT_RESISTOR    0.020f  // 20mOhm shunt resistor
#define BQ_CURRENT_LSB    0.000422f  // (8.44µV / 0.020Ω) = 0.422mA per LSB

// NTC Thermistor Parameters (10k NTC with Beta = 3950)
#define NTC_R0            10000.0f  // Resistance at 25°C (Ohms)
#define NTC_T0            298.15f   // 25°C in Kelvin
#define NTC_BETA          3950.0f   // Beta coefficient
#define NTC_SERIES_R      10000.0f  // Series resistor value (Ohms)
#define NTC_VCC           3.3f      // Supply voltage
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;
DMA_HandleTypeDef hdma_adc1;
DMA_HandleTypeDef hdma_adc2;

I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN PV */
char data_buffer[256];
uint8_t bms_addr = 0;
uint8_t use_crc = 0; // 0 = No CRC, 1 = Use CRC
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);
static void MX_ADC2_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// --- ADC READING HELPER ---
// Configures a specific channel, takes a single reading, and returns the 12-bit value
uint32_t Read_ADC(ADC_HandleTypeDef* hadc, uint32_t channel) {
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    // Slower sampling time allows stable readings for high-impedance NTC thermistors
    sConfig.SamplingTime = ADC_SAMPLETIME_601CYCLES_5;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;

    // If it fails to configure, return a specific error code
    if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK) return 99999;

    HAL_ADC_Start(hadc);
    if (HAL_ADC_PollForConversion(hadc, 50) == HAL_OK) { // 50ms timeout
        uint32_t val = HAL_ADC_GetValue(hadc);
        HAL_ADC_Stop(hadc);
        return val;
    }
    HAL_ADC_Stop(hadc);
    // If it times out, return a specific error code
    return 88888;
}

// Converts ADC raw value to Voltage, handling our custom error codes
float convert_adc(uint32_t raw) {
    if (raw == 99999) return 99.9f; // Hardware Config Error
    if (raw == 88888) return 88.8f; // Timeout Error
    return raw * 3.3f / 4095.0f;    // Normal Voltage Conversion
}

// --- TEMPERATURE CONVERSION ---
// Converts voltage from NTC voltage divider to temperature in Celsius
float voltage_to_temperature(float voltage) {
    // Handle error codes
    if (voltage >= 99.0f) return 999.9f;  // Error - sensor fault
    if (voltage >= 88.0f) return 888.8f;  // Error - timeout

    // Avoid division by zero
    if (voltage <= 0.01f) return -273.0f;  // Error - disconnected
    if (voltage >= (NTC_VCC - 0.01f)) return -274.0f;  // Error - shorted

    // Calculate NTC resistance from voltage divider
    // Vout = Vcc * (R_NTC / (R_series + R_NTC))
    // R_NTC = (Vout * R_series) / (Vcc - Vout)
    float r_ntc = (voltage * NTC_SERIES_R) / (NTC_VCC - voltage);

    // Simplified Beta equation: 1/T = 1/T0 + (1/Beta)*ln(R/R0)
    float steinhart = (1.0f / NTC_T0) + (1.0f / NTC_BETA) * logf(r_ntc / NTC_R0);
    float temp_kelvin = 1.0f / steinhart;
    float temp_celsius = temp_kelvin - 273.15f;

    return temp_celsius;
}

// --- CRC8 CALCULATION (Polynomial 0x07) ---
uint8_t CRC8(uint8_t *data, int len) {
    uint8_t crc = 0;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) crc = (crc << 1) ^ 0x07;
            else crc <<= 1;
        }
    }
    return crc;
}

// --- WRITE REGISTER (Handles CRC automatically) ---
void BQ_WriteReg(uint8_t reg, uint8_t data) {
    if (bms_addr == 0) return;

    if (use_crc) {
        uint8_t tx[4];
        tx[0] = bms_addr;
        tx[1] = reg;
        tx[2] = data;
        uint8_t crc_payload[3] = {bms_addr, reg, data};
        tx[3] = CRC8(crc_payload, 3);
        HAL_I2C_Master_Transmit(&hi2c1, bms_addr, &tx[1], 3, 100);
    } else {
        uint8_t tx[2] = {reg, data};
        HAL_I2C_Master_Transmit(&hi2c1, bms_addr, tx, 2, 100);
    }
}

// --- READ REGISTERS (Handles CRC automatically) ---
void BQ_ReadRegs(uint8_t reg, uint8_t *data, uint16_t count) {
    if (bms_addr == 0) return;

    if (use_crc) {
        uint8_t rx_buffer[128];
        HAL_I2C_Mem_Read(&hi2c1, bms_addr, reg, I2C_MEMADD_SIZE_8BIT, rx_buffer, count * 2, 100);
        for(int i=0; i<count; i++) {
            data[i] = rx_buffer[i*2];
        }
    } else {
        HAL_I2C_Mem_Read(&hi2c1, bms_addr, reg, I2C_MEMADD_SIZE_8BIT, data, count, 100);
    }
}

// --- READ CURRENT FROM BQ76930 ---
float BQ_ReadCurrent(void) {
    if (bms_addr == 0) return 0.0f;

    uint8_t raw[2] = {0};
    BQ_ReadRegs(CC_HI_BYTE, raw, 2);

    // Combine high and low bytes (signed 16-bit)
    int16_t current_raw = (int16_t)((raw[0] << 8) | raw[1]);

    // Convert to Amperes using 20mOhm shunt
    // BQ76930: 8.44µV per LSB, Shunt: 20mOhm
    // Current = (8.44e-6 V) / (0.020 Ω) = 0.422mA per LSB
    float current_amps = current_raw * BQ_CURRENT_LSB;

    return current_amps;
}

void BQ_Init(void) {
    if (bms_addr == 0) return;
    BQ_WriteReg(SYS_STAT, 0xFF);
    BQ_WriteReg(SYS_CTRL1, 0x10);
    BQ_WriteReg(SYS_CTRL2, 0x40);
}

// --- SCANNER ---
void Discover_BMS(void) {
    HAL_StatusTypeDef res;

    res = HAL_I2C_IsDeviceReady(&hi2c1, 0x08 << 1, 2, 10);
    if (res == HAL_OK) {
        bms_addr = 0x08 << 1;
        use_crc = 0;
        return;
    }

    res = HAL_I2C_IsDeviceReady(&hi2c1, 0x18 << 1, 2, 10);
    if (res == HAL_OK) {
        bms_addr = 0x18 << 1;
        use_crc = 0;
        return;
    }
    bms_addr = 0;
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

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USB_DEVICE_Init();
  MX_I2C1_Init();
  MX_ADC1_Init();
  MX_ADC2_Init();
  /* USER CODE BEGIN 2 */

  // USB D+ pull-up on PC2 is now handled automatically by
  // HAL_PCDEx_SetConnectionState() in usbd_conf.c — called
  // by the USB stack during USBD_Start() inside MX_USB_DEVICE_Init().
  // Allow time for Windows to complete enumeration before proceeding.
  HAL_Delay(500);

  // Initial Scan
  Discover_BMS();

  if (bms_addr != 0) {
      BQ_Init();
      HAL_Delay(100);
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE BEGIN WHILE_LOOP_LOGIC */

    if (bms_addr == 0) {
        Discover_BMS();
        int len = snprintf(data_buffer, sizeof(data_buffer), "SCANNING... (Press Boot Button)\r\n");
        CDC_Transmit_FS((uint8_t*)data_buffer, len);
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_11);
        HAL_Delay(500);
        if (bms_addr != 0) BQ_Init();
        continue;
    }

    // --- 1. READ BMS CELL VOLTAGES ---
    uint8_t raw[20] = {0};
    BQ_ReadRegs(VC1_HI_BYTE, raw, 20);

    float v[10];
    for (int i = 0; i < 10; i++) {
        int16_t adc = (int16_t)((raw[i*2] << 8) | raw[i*2+1]) & 0x3FFF;
        v[i] = adc * 382.0f / 1000000.0f;
    }

    // --- 2. READ CURRENT FROM BQ76930 ---
    float current = BQ_ReadCurrent();

    if (v[0] < 0.1f) {
        if (use_crc == 0) {
            use_crc = 1;
            BQ_Init();
        } else {
             int len = snprintf(data_buffer, sizeof(data_buffer),
                 "ADDR: 0x%02X (CRC ON) | V=0.000 (Check Voltages)\r\n", bms_addr);
             CDC_Transmit_FS((uint8_t*)data_buffer, len);
        }
    } else {
        // SUCCESS - Format all 10 cells with current
        int len = snprintf(data_buffer, sizeof(data_buffer),
            "C1:%.3f C2:%.3f C3:%.3f C4:%.3f C5:%.3f C6:%.3f C7:%.3f C8:%.3f C9:%.3f C10:%.3f | I:%.3fA\r\n",
            v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], current);
        CDC_Transmit_FS((uint8_t*)data_buffer, len);
    }

    HAL_Delay(5); // Tiny delay to let USB flush previous print

    // --- 3. READ STM32 THERMISTORS & CONVERT TO TEMPERATURE ---
    float t_voltage[10];
    float t_celsius[10];

    t_voltage[0] = convert_adc(Read_ADC(&hadc1, ADC_CHANNEL_1)); // PA0 (ADC1 IN1)
    t_voltage[1] = convert_adc(Read_ADC(&hadc1, ADC_CHANNEL_2)); // PA1 (ADC1 IN2)
    t_voltage[2] = convert_adc(Read_ADC(&hadc1, ADC_CHANNEL_3)); // PA2 (ADC1 IN3)
    t_voltage[3] = convert_adc(Read_ADC(&hadc1, ADC_CHANNEL_4)); // PA3 (ADC1 IN4)
    t_voltage[4] = convert_adc(Read_ADC(&hadc1, ADC_CHANNEL_5)); // PF4 (ADC1 IN5)
    t_voltage[5] = convert_adc(Read_ADC(&hadc2, ADC_CHANNEL_1)); // PA4 (ADC2 IN1)
    t_voltage[6] = convert_adc(Read_ADC(&hadc2, ADC_CHANNEL_2)); // PA5 (ADC2 IN2)
    t_voltage[7] = convert_adc(Read_ADC(&hadc2, ADC_CHANNEL_3)); // PA6 (ADC2 IN3)
    t_voltage[8] = convert_adc(Read_ADC(&hadc2, ADC_CHANNEL_4)); // PA7 (ADC2 IN4)
    t_voltage[9] = convert_adc(Read_ADC(&hadc2, ADC_CHANNEL_5)); // PC4 (ADC2 IN5)

    // Convert voltages to temperatures
    for (int i = 0; i < 10; i++) {
        t_celsius[i] = voltage_to_temperature(t_voltage[i]);
    }

    // Format all 10 Temperatures in Celsius
    int len2 = snprintf(data_buffer, sizeof(data_buffer),
        "T1:%.1f T2:%.1f T3:%.1f T4:%.1f T5:%.1f T6:%.1f T7:%.1f T8:%.1f T9:%.1f T10:%.1f °C\r\n\n",
        t_celsius[0], t_celsius[1], t_celsius[2], t_celsius[3], t_celsius[4],
        t_celsius[5], t_celsius[6], t_celsius[7], t_celsius[8], t_celsius[9]);
    CDC_Transmit_FS((uint8_t*)data_buffer, len2);

    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_11);
    HAL_Delay(500);

    /* USER CODE END WHILE_LOOP_LOGIC */
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV5;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL8;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB|RCC_PERIPHCLK_I2C1
                              |RCC_PERIPHCLK_ADC12;
  PeriphClkInit.Adc12ClockSelection = RCC_ADC12PLLCLK_DIV1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  PeriphClkInit.USBClockSelection = RCC_USBCLKSOURCE_PLL;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_RCC_MCOConfig(RCC_MCO, RCC_MCO1SOURCE_HSE, RCC_MCODIV_1);
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 5;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = ADC_REGULAR_RANK_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief ADC2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC2_Init(void)
{

  /* USER CODE BEGIN ADC2_Init 0 */

  /* USER CODE END ADC2_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC2_Init 1 */

  /* USER CODE END ADC2_Init 1 */

  /** Common config
  */
  hadc2.Instance = ADC2;
  hadc2.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc2.Init.Resolution = ADC_RESOLUTION_12B;
  hadc2.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc2.Init.ContinuousConvMode = ENABLE;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc2.Init.NbrOfConversion = 5;
  hadc2.Init.DMAContinuousRequests = DISABLE;
  hadc2.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc2.Init.LowPowerAutoWait = DISABLE;
  hadc2.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_4;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = ADC_REGULAR_RANK_5;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC2_Init 2 */

  /* USER CODE END ADC2_Init 2 */

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
  hi2c1.Init.Timing = 0x00201D2B;
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
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA2_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Channel1_IRQn);

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
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, USB_Pul__Up_Pin|LED_HEARTBEAT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : USB_Pul__Up_Pin LED_HEARTBEAT_Pin */
  GPIO_InitStruct.Pin = USB_Pul__Up_Pin|LED_HEARTBEAT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF0_MCO;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
