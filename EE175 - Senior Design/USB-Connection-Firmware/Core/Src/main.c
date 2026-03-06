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
#include <string.h>
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
#define PROTECT2          0x07  // Overcurrent trip delay & threshold
#define ADCGAIN1          0x50
#define ADCGAIN2          0x59
#define ADCOFFSET         0x51
#define OV_TRIP           0x09
#define UV_TRIP           0x0A
#define CELLBAL1          0x01
#define CELLBAL2          0x02

#define EVENS1             0x0A
#define ODDS1              0x15
#define EVENS2             0x15
#define ODDS2              0x0A

#define BAL_ALT_PERIOD_MS 5000      // 5-second alternation period

// Current Shunt Parameters
#define SHUNT_RESISTOR    0.020f  // 20mOhm shunt resistor
#define BQ_CURRENT_LSB    0.000422f  // (8.44µV / 0.020Ω) = 0.422mA per LSB

// NTC Thermistor Parameters (10k NTC with Beta = 3950)
#define NTC_R0            10000.0f  // Resistance at 25°C (Ohms)
#define NTC_T0            298.15f   // 25°C in Kelvin
#define NTC_BETA          3950.0f   // Beta coefficient
#define NTC_SERIES_R      10000.0f  // Series resistor value (Ohms)
#define NTC_VCC           3.3f      // Supply voltage

// Fan Control Parameters
#define FAN_PWM_MAX       999       // TIM17 ARR value (0-999 = 0-100% duty)
#define FAN_TACH_TIMER_FREQ 1000000 // TIM3 tick rate: 48MHz / (47+1) = 1MHz
#define FAN_PULSES_PER_REV  2       // Most fans output 2 pulses per revolution
#define FAN_RPM_TIMEOUT_MS  2000    // If no tach pulse for 2s, fan is stopped/stalled
#define FAN_RPM_AVG_SAMPLES 4       // Moving average window size
#define FAN_RPM_MAX         15000   // Max plausible RPM (reject noise above this)
#define FAN_RPM_RATED       2700    // Fan's maximum rated RPM at 100% duty
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

TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim17;

/* USER CODE BEGIN PV */
char data_buffer[640];
uint8_t bms_addr = 0;
uint8_t use_crc = 0; // 0 = No CRC, 1 = Use CRC

// Fan Control Variables
volatile uint32_t fan_tach_capture_last = 0;   // Previous capture value
volatile uint32_t fan_tach_period = 0;         // Period between falling edges (in µs)
volatile uint8_t  fan_tach_new_data = 0;       // Flag: new capture available
volatile uint32_t fan_tach_last_tick = 0;      // HAL_GetTick() at last capture (for timeout)
volatile uint16_t fan_tach_overflow_count = 0; // Timer overflow count (for low-RPM measurement)
uint16_t fan_duty = 0;                         // Current PWM duty (0-999)
uint32_t fan_rpm = 0;                          // Calculated RPM
uint32_t fan_rpm_buf[FAN_RPM_AVG_SAMPLES];     // Moving average buffer
uint8_t  fan_rpm_idx = 0;                      // Current index into averaging buffer
uint8_t  fan_rpm_filled = 0;                   // 1 once buffer has been fully populated
uint8_t  fan_auto_mode = 1;                    // 1 = auto (temp-based), 0 = manual (dashboard controls)
uint8_t  fan_manual_duty = 0;                  // Manual duty cycle set by dashboard (0-100%)

// Cell Balancing Variables
uint8_t  bal_enabled = 0;           // 0 = off, 1 = on
uint16_t bal_threshold_mv = 15;     // delta threshold in mV
uint16_t bal_active_mask = 0;       // bitmask: bit 0=cell1 ... bit 9=cell10
#define BAL_MIN_CELL_MV  2850       // no balancing below 2.85V
#define BAL_MAX_TEMP_C   50.0f      // no balancing above 50C
uint8_t  bal_alt_enabled = 0;       // 0 = off, 1 = alternating even/odd mode
uint8_t  bal_alt_phase = 0;         // 0 = odd cells, 1 = even cells
uint32_t bal_alt_last_toggle = 0;   // HAL_GetTick() of last phase switch

// Charge Mode Variables
uint8_t  charge_mode = 0;           // 0 = off (discharge mode), 1 = charge mode
uint8_t  thermal_shutdown = 0;      // 1 = FETs disabled due to overtemp
#define  CHARGE_BAL_THRESHOLD 3.800f // balance cells above 3.8V during charging
#define  THERMAL_CUTOFF_C    60.0f   // disable FETs above 60C
#define  THERMAL_RESUME_C    55.0f   // re-enable FETs below 55C

// USB Command receive (from usbd_cdc_if.c)
extern volatile uint8_t cmd_ready[];
extern volatile uint8_t cmd_ready_flag;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);
static void MX_ADC2_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM17_Init(void);
/* USER CODE BEGIN PFP */
void Fan_SetSpeed(uint8_t percent);
uint32_t Fan_GetRPM(void);
void Process_USB_Command(const char *cmd);
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
    BQ_WriteReg(SYS_CTRL2, 0xC0); // 0x40 if you want to re-enable OV, UV, OCD, & SCD delays

}

// --- CELL BALANCING ---
void BQ_UpdateBalance(float *cell_v, int cell_count, float max_temp) {
    bal_active_mask = 0;

    if (!bal_enabled) {
        BQ_WriteReg(CELLBAL1, 0x00);
        BQ_WriteReg(CELLBAL2, 0x00);
        return;
    }

    // Find minimum cell voltage
    float v_min = cell_v[0];
    for (int i = 1; i < cell_count; i++) {
        if (cell_v[i] < v_min) v_min = cell_v[i];
    }

    // Safety: abort if lowest cell below minimum or temperature too high
    float v_min_mv = v_min * 1000.0f;
    if (v_min_mv < (float)BAL_MIN_CELL_MV || max_temp > BAL_MAX_TEMP_C) {
        BQ_WriteReg(CELLBAL1, 0x00);
        BQ_WriteReg(CELLBAL2, 0x00);
        return;
    }

    float threshold_v = (float)bal_threshold_mv / 1000.0f;
    uint8_t cellbal1 = 0;  // bits [4:0] for cells 1-5
    uint8_t cellbal2 = 0;  // bits [4:0] for cells 6-10

    for (int i = 0; i < cell_count; i++) {
        if ((cell_v[i] - v_min) > threshold_v) {
            bal_active_mask |= (1 << i);
            if (i < 5) {
                cellbal1 |= (1 << i);
            } else {
                cellbal2 |= (1 << (i - 5));
            }
        }
    }

    BQ_WriteReg(CELLBAL1, cellbal1);
    BQ_WriteReg(CELLBAL2, cellbal2);
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
  MX_TIM3_Init();
  MX_TIM17_Init();
  /* USER CODE BEGIN 2 */

  // USB D+ pull-up on PC2 is now handled automatically by
  // HAL_PCDEx_SetConnectionState() in usbd_conf.c — called
  // by the USB stack during USBD_Start() inside MX_USB_DEVICE_Init().
  // Allow time for Windows to complete enumeration before proceeding.
  HAL_Delay(500);

  // --- Start Fan PWM (TIM17 CH1 on PB9) ---
  HAL_TIM_PWM_Start(&htim17, TIM_CHANNEL_1);
  __HAL_TIM_MOE_ENABLE(&htim17);   // Force Main Output Enable for advanced timer
  Fan_SetSpeed(100);  // Fan at 100% for testing

  // --- Start Fan Tach Input Capture (TIM3 CH1 on PC6) ---
  // Re-configure PC6 with internal pull-up (tach is open-drain)
  {
      GPIO_InitTypeDef gpio = {0};
      gpio.Pin = GPIO_PIN_6;
      gpio.Mode = GPIO_MODE_AF_PP;
      gpio.Pull = GPIO_PULLUP;
      gpio.Speed = GPIO_SPEED_FREQ_LOW;
      gpio.Alternate = GPIO_AF2_TIM3;
      HAL_GPIO_Init(GPIOC, &gpio);
  }
  HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);
  __HAL_TIM_ENABLE_IT(&htim3, TIM_IT_UPDATE);  // Enable overflow interrupt for low-RPM tracking
  fan_tach_last_tick = HAL_GetTick();

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

    // --- 0. CHECK FOR USB COMMANDS FROM DASHBOARD ---
    if (cmd_ready_flag) {
        Process_USB_Command((const char *)cmd_ready);
        cmd_ready_flag = 0;
    }

    uint8_t faultStatus[1] = {0};
    BQ_ReadRegs(SYS_STAT, faultStatus, 1);

	uint8_t loadPresent[1] = {0};
	BQ_ReadRegs(SYS_CTRL1, loadPresent, 1);
	loadPresent[0] = loadPresent[0] & 0x80;

	// --- -1. SET OVERVOLTAGE & UNDERVOLTAGE TRIP TRESHOLDS ---
	BQ_WriteReg(OV_TRIP, 0x8E); // 3.8V is 0x6D Set to 4V (0x8E) see p. 22 in ds
	BQ_WriteReg(UV_TRIP, 0xB9); // Set to 2.7V

	// --- 0. SET OVERCURRENT TRIP DELAY & THRESHOLD ---
	BQ_WriteReg(PROTECT2, 0x03); // 8 ms delay & 17mV across shunt

	// --- 0.5 FET CONTROL (moved after temp reads for thermal safety) ---

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

    // CRC detection: only switch if ALL cells read zero
    {
        uint8_t all_zero = 1;
        for (int i = 0; i < 10; i++) {
            if (v[i] > 0.1f) { all_zero = 0; break; }
        }
        if (all_zero) {
            if (use_crc == 0) {
                use_crc = 1;
                BQ_Init();
            } else {
                int len = snprintf(data_buffer, sizeof(data_buffer),
                    "ADDR: 0x%02X (CRC ON) | V=0.000 (Check Voltages)\r\n", bms_addr);
                CDC_Transmit_FS((uint8_t*)data_buffer, len);
            }
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_11);
            HAL_Delay(500);
            continue;
        }
    }

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

    // --- 3.5 THERMAL SAFETY & FET CONTROL ---
    {
        // Find max valid temperature
        float max_valid_temp = -300.0f;
        for (int i = 0; i < 10; i++) {
            if (t_celsius[i] > -100.0f && t_celsius[i] < 200.0f && t_celsius[i] > max_valid_temp) {
                max_valid_temp = t_celsius[i];
            }
        }

        if (max_valid_temp > THERMAL_CUTOFF_C) {
            // OVERTEMP: disable both FETs immediately
            thermal_shutdown = 1;
            BQ_WriteReg(SYS_CTRL2, 0xC0);
            BQ_WriteReg(CELLBAL1, 0x00);
            BQ_WriteReg(CELLBAL2, 0x00);
            bal_active_mask = 0;
        } else if (thermal_shutdown && max_valid_temp < THERMAL_RESUME_C && max_valid_temp > -100.0f) {
            // Temp dropped below resume threshold — clear shutdown
            thermal_shutdown = 0;
        }

        if (!thermal_shutdown) {
            if (charge_mode) {
                BQ_WriteReg(SYS_CTRL2, 0xC1); // charge ON, discharge OFF
            } else {
                BQ_WriteReg(SYS_CTRL2, 0xC2); // discharge ON, charge OFF
            }
        }
    }

    // --- 3.6 CELL BALANCING ---
    static uint32_t last_balance_eval = 0;
    if ((HAL_GetTick() - last_balance_eval) >= 5000) {
        last_balance_eval = HAL_GetTick();

        bal_active_mask = 0;
        if (thermal_shutdown) {
            // No balancing during thermal shutdown
            BQ_WriteReg(CELLBAL1, 0x00);
            BQ_WriteReg(CELLBAL2, 0x00);
        } else if (charge_mode) {
            // CHARGE MODE: balance cells above 3.8V with smart even/odd alternation
            uint8_t cellbal1 = 0, cellbal2 = 0;
            uint16_t need_bal = 0;  // bitmask of cells needing balance
            for (int i = 0; i < 10; i++) {
                if (v[i] > CHARGE_BAL_THRESHOLD) {
                    need_bal |= (1 << i);
                    if (i < 5) cellbal1 |= (1 << i);
                    else cellbal2 |= (1 << (i - 5));
                }
            }

            if (need_bal == 0) {
                // No cells above 3.8V — nothing to balance
                BQ_WriteReg(CELLBAL1, 0x00);
                BQ_WriteReg(CELLBAL2, 0x00);
            } else {
                // Check if we have both even and odd cells needing balance
                uint8_t has_odd  = (need_bal & 0x0155) ? 1 : 0; // bits 0,2,4,6,8
                uint8_t has_even = (need_bal & 0x02AA) ? 1 : 0; // bits 1,3,5,7,9

                if (has_odd && has_even) {
                    // Mixed: alternate every 5 seconds
                    if ((HAL_GetTick() - bal_alt_last_toggle) >= BAL_ALT_PERIOD_MS) {
                        bal_alt_phase ^= 1;
                        bal_alt_last_toggle = HAL_GetTick();
                    }
                    if (bal_alt_phase == 0) {
                        cellbal1 &= ODDS1;
                        cellbal2 &= ODDS2;
                    } else {
                        cellbal1 &= EVENS1;
                        cellbal2 &= EVENS2;
                    }
                } else if (has_odd && !has_even) {
                    // Only odd — balance directly, but prep phase for when evens appear
                    cellbal1 &= ODDS1;
                    cellbal2 &= ODDS2;
                    bal_alt_phase = 1;  // next mixed transition starts with evens
                    bal_alt_last_toggle = HAL_GetTick();
                } else {
                    // Only even — balance directly
                    cellbal1 &= EVENS1;
                    cellbal2 &= EVENS2;
                    bal_alt_phase = 0;  // next mixed transition starts with odds
                    bal_alt_last_toggle = HAL_GetTick();
                }

                // Update active mask
                bal_active_mask = 0;
                for (int i = 0; i < 5; i++) {
                    if (cellbal1 & (1 << i)) bal_active_mask |= (1 << i);
                    if (cellbal2 & (1 << i)) bal_active_mask |= (1 << (i + 5));
                }

                BQ_WriteReg(CELLBAL1, cellbal1);
                BQ_WriteReg(CELLBAL2, cellbal2);
            }
        } else if (bal_enabled && bal_alt_enabled) {
            // BALANCE CELLS MODE: threshold + alternating even/odd
            if ((HAL_GetTick() - bal_alt_last_toggle) >= BAL_ALT_PERIOD_MS) {
                bal_alt_phase ^= 1;
                bal_alt_last_toggle = HAL_GetTick();
            }

            float v_min = 99.0f;
            for (int i = 0; i < 10; i++) {
                if (v[i] > 0.5f && v[i] < v_min) v_min = v[i];
            }

            float threshold_v = (float)bal_threshold_mv / 1000.0f;
            uint8_t cellbal1 = 0, cellbal2 = 0;
            if (v_min < 98.0f) {
                for (int i = 0; i < 10; i++) {
                    if (v[i] < 0.5f) continue;
                    if ((v[i] - v_min) > threshold_v) {
                        if (i < 5) cellbal1 |= (1 << i);
                        else cellbal2 |= (1 << (i - 5));
                    }
                }
            }

            if (bal_alt_phase == 0) {
                cellbal1 &= ODDS1;
                cellbal2 &= ODDS2;
            } else {
                cellbal1 &= EVENS1;
                cellbal2 &= EVENS2;
            }

            bal_active_mask = 0;
            for (int i = 0; i < 5; i++) {
                if (cellbal1 & (1 << i)) bal_active_mask |= (1 << i);
                if (cellbal2 & (1 << i)) bal_active_mask |= (1 << (i + 5));
            }

            BQ_WriteReg(CELLBAL1, cellbal1);
            BQ_WriteReg(CELLBAL2, cellbal2);
        } else {
            BQ_WriteReg(CELLBAL1, 0x00);
            BQ_WriteReg(CELLBAL2, 0x00);
        }
    }

    // --- 4. FAN CONTROL (Auto or Manual) ---
    uint8_t effective_duty;
    if (fan_auto_mode) {
        // Find MAX temperature across all 10 NTCs
        float max_temp = t_celsius[0];
        for (int i = 1; i < 10; i++) {
            if (t_celsius[i] > max_temp) max_temp = t_celsius[i];
        }

        // Temperature -> Duty mapping:
        //   <= 25C  -> 0% (fan off, cells are cool)
        //   25-45C  -> linear ramp 20%-100%
        //   >= 45C  -> 100% (full blast)
        if (max_temp <= 25.0f) {
            effective_duty = 0;
        } else if (max_temp >= 45.0f) {
            effective_duty = 100;
        } else {
            effective_duty = (uint8_t)(20 + (max_temp - 25.0f) * 80.0f / 20.0f);
        }
    } else {
        // Manual mode: use duty set by dashboard
        effective_duty = fan_manual_duty;
    }
    Fan_SetSpeed(effective_duty);

    // Read real tach RPM; fall back to estimated RPM if tach times out
    fan_rpm = Fan_GetRPM();
    if (fan_rpm == 0 && effective_duty > 0) {
        fan_rpm = (uint32_t)effective_duty * FAN_RPM_RATED / 100;
    }

    // --- 5. SEND HUMAN-READABLE OUTPUT ---
    int len = snprintf(data_buffer, sizeof(data_buffer),
        "Voltages ->  C1: %.3f,  C2: %.3f,  C3: %.3f,  C4: %.3f,  C5: %.3f,  C6: %.3f,  C7: %.3f,  C8: %.3f,  C9: %.3f,  C10: %.3f\r\n"
        "Temperatures -> T1: %.1f,  T2: %.1f,  T3: %.1f,  T4: %.1f,  T5: %.1f,  T6: %.1f,  T7: %.1f,  T8: %.1f,  T9: %.1f,  T10: %.1f\r\n"
        "Current ->  %.3f A\r\n"
        "Fan ->  %lu RPM\r\n"
        "fan_auto:%d fan_duty:%d\r\n"
    	"SYS_STAT:%x Load Present:%x\r\n"
        "bal_en:%d bal_thresh:%d bal_mask:%d bal_alt:%d charge:%d\r\n\r\n",
        v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9],
        t_celsius[0], t_celsius[1], t_celsius[2], t_celsius[3], t_celsius[4],
        t_celsius[5], t_celsius[6], t_celsius[7], t_celsius[8], t_celsius[9],
        current,
        fan_rpm,
        fan_auto_mode, (int)effective_duty,
        faultStatus[0], loadPresent[0],
        (int)bal_enabled, (int)bal_threshold_mv, (int)bal_active_mask, (int)bal_alt_enabled, (int)charge_mode);
    CDC_Transmit_FS((uint8_t*)data_buffer, len);

    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_11);

    // Wait ~500ms for next telemetry cycle, but check for USB commands every 10ms
    {
        uint32_t wait_start = HAL_GetTick();
        while ((HAL_GetTick() - wait_start) < 500) {
            if (cmd_ready_flag) {
                Process_USB_Command((const char *)cmd_ready);
                cmd_ready_flag = 0;
            }
            HAL_Delay(10);
        }
    }

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
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 47;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0x0F;
  if (HAL_TIM_IC_ConfigChannel(&htim3, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief TIM17 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM17_Init(void)
{

  /* USER CODE BEGIN TIM17_Init 0 */

  /* USER CODE END TIM17_Init 0 */

  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM17_Init 1 */

  /* USER CODE END TIM17_Init 1 */
  htim17.Instance = TIM17;
  htim17.Init.Prescaler = 47;
  htim17.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim17.Init.Period = 999;
  htim17.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim17.Init.RepetitionCounter = 0;
  htim17.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim17) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim17) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim17, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim17, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM17_Init 2 */

  /* USER CODE END TIM17_Init 2 */
  HAL_TIM_MspPostInit(&htim17);

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init — lower priority than USB (which is 0,0) */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA2_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Channel1_IRQn, 1, 0);
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

// --- USB COMMAND PROCESSING ---
// Handles commands from the BMS Dashboard sent over USB CDC
void Process_USB_Command(const char *cmd) {
    // FAN:AUTO — switch to automatic temperature-based control
    if (strcmp(cmd, "FAN:AUTO") == 0) {
        fan_auto_mode = 1;
        return;
    }

    // FAN:MANUAL — switch to manual dashboard control
    if (strcmp(cmd, "FAN:MANUAL") == 0) {
        fan_auto_mode = 0;
        return;
    }

    // FAN:SET:XX — set manual duty cycle (0-100%)
    if (strncmp(cmd, "FAN:SET:", 8) == 0) {
        int duty = atoi(cmd + 8);
        if (duty < 0) duty = 0;
        if (duty > 100) duty = 100;
        fan_manual_duty = (uint8_t)duty;
        if (!fan_auto_mode) {
            Fan_SetSpeed(fan_manual_duty);
        }
        return;
    }

    // BMS:CLEAR_FAULTS — write 0x7F to SYS_STAT to clear fault bits [6:0], preserve bit 7
    if (strcmp(cmd, "BMS:CLEAR_FAULTS") == 0) {
        BQ_WriteReg(SYS_STAT, 0x7F);
        return;
    }

    // BMS:BAL:OFF — disable all cell balancing
    if (strcmp(cmd, "BMS:BAL:OFF") == 0) {
        bal_enabled = 0;
        bal_alt_enabled = 0;
        bal_active_mask = 0;
        BQ_WriteReg(CELLBAL1, 0x00);
        BQ_WriteReg(CELLBAL2, 0x00);
        return;
    }

    // BMS:BAL:ALT:ON — enable alternating even/odd cell balancing
    if (strcmp(cmd, "BMS:BAL:ALT:ON") == 0) {
        bal_enabled = 1;
        bal_alt_enabled = 1;
        bal_alt_phase = 0;
        bal_alt_last_toggle = HAL_GetTick();
        return;
    }

    // BMS:BAL:ALT:OFF — disable alternating cell balancing
    if (strcmp(cmd, "BMS:BAL:ALT:OFF") == 0) {
        bal_alt_enabled = 0;
        bal_enabled = 0;
        bal_active_mask = 0;
        BQ_WriteReg(CELLBAL1, 0x00);
        BQ_WriteReg(CELLBAL2, 0x00);
        return;
    }

    // BMS:BAL:THRESH:XX — set balance threshold in mV (5-100)
    if (strncmp(cmd, "BMS:BAL:THRESH:", 15) == 0) {
        int thresh = atoi(cmd + 15);
        if (thresh >= 5 && thresh <= 100) {
            bal_threshold_mv = (uint16_t)thresh;
        }
        return;
    }

    // BMS:CHARGE:ON — enable charge mode (charge FET on, discharge FET off)
    if (strcmp(cmd, "BMS:CHARGE:ON") == 0) {
        charge_mode = 1;
        thermal_shutdown = 0;
        // Disable manual balancing when entering charge mode
        bal_enabled = 0;
        bal_alt_enabled = 0;
        return;
    }

    // BMS:CHARGE:OFF — disable charge mode (back to discharge mode)
    if (strcmp(cmd, "BMS:CHARGE:OFF") == 0) {
        charge_mode = 0;
        thermal_shutdown = 0;
        bal_active_mask = 0;
        BQ_WriteReg(CELLBAL1, 0x00);
        BQ_WriteReg(CELLBAL2, 0x00);
        BQ_WriteReg(SYS_CTRL2, 0xC0); // both FETs off until next loop sets correct mode
        return;
    }
}

// --- FAN CONTROL FUNCTIONS ---

// Set fan speed: 0-100 percent
// At 100%: switch PB9 to GPIO output HIGH for solid ground path (clean tach signal)
// At 0%:   switch PB9 to GPIO output LOW (fan off)
// Otherwise: use TIM17 PWM
void Fan_SetSpeed(uint8_t percent) {
    if (percent > 100) percent = 100;
    fan_duty = (uint16_t)((uint32_t)percent * FAN_PWM_MAX / 100);

    if (percent == 100) {
        // Bypass PWM — drive gate fully HIGH for clean ground path
        GPIO_InitTypeDef gpio = {0};
        gpio.Pin = FAN_EN_Pin;
        gpio.Mode = GPIO_MODE_OUTPUT_PP;
        gpio.Pull = GPIO_NOPULL;
        gpio.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(FAN_EN_GPIO_Port, &gpio);
        HAL_GPIO_WritePin(FAN_EN_GPIO_Port, FAN_EN_Pin, GPIO_PIN_SET);
    } else if (percent == 0) {
        // Drive gate fully LOW — fan off
        GPIO_InitTypeDef gpio = {0};
        gpio.Pin = FAN_EN_Pin;
        gpio.Mode = GPIO_MODE_OUTPUT_PP;
        gpio.Pull = GPIO_NOPULL;
        gpio.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(FAN_EN_GPIO_Port, &gpio);
        HAL_GPIO_WritePin(FAN_EN_GPIO_Port, FAN_EN_Pin, GPIO_PIN_RESET);
    } else {
        // Restore PWM alternate function for variable speed
        GPIO_InitTypeDef gpio = {0};
        gpio.Pin = FAN_EN_Pin;
        gpio.Mode = GPIO_MODE_AF_PP;
        gpio.Pull = GPIO_NOPULL;
        gpio.Speed = GPIO_SPEED_FREQ_LOW;
        gpio.Alternate = GPIO_AF1_TIM17;
        HAL_GPIO_Init(FAN_EN_GPIO_Port, &gpio);
        __HAL_TIM_SET_COMPARE(&htim17, TIM_CHANNEL_1, fan_duty);
    }
}

// Get current fan RPM with moving average (call periodically from main loop)
uint32_t Fan_GetRPM(void) {
    // Check for timeout (fan stopped or disconnected)
    if ((HAL_GetTick() - fan_tach_last_tick) > FAN_RPM_TIMEOUT_MS) {
        fan_rpm_filled = 0;
        fan_rpm_idx = 0;
        return 0;
    }

    if (fan_tach_period == 0) return 0;

    // RPM = (60 * timer_freq) / (period_ticks * pulses_per_rev)
    uint32_t raw_rpm = (60UL * FAN_TACH_TIMER_FREQ) / (fan_tach_period * FAN_PULSES_PER_REV);

    // Reject noise spikes above max plausible RPM
    if (raw_rpm > FAN_RPM_MAX) return 0;

    // Store in moving average buffer
    fan_rpm_buf[fan_rpm_idx] = raw_rpm;
    fan_rpm_idx = (fan_rpm_idx + 1) % FAN_RPM_AVG_SAMPLES;
    if (!fan_rpm_filled && fan_rpm_idx == 0) fan_rpm_filled = 1;

    // Compute average over available samples
    uint8_t count = fan_rpm_filled ? FAN_RPM_AVG_SAMPLES : fan_rpm_idx;
    if (count == 0) return raw_rpm;

    uint32_t sum = 0;
    for (uint8_t i = 0; i < count; i++) {
        sum += fan_rpm_buf[i];
    }
    return sum / count;
}

// HAL Timer overflow callback — counts overflows for low-RPM measurement
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM3) {
        fan_tach_overflow_count++;
    }
}

// HAL Input Capture callback — called by HAL from TIM3 ISR
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM3 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
        uint32_t capture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);

        // Calculate period including overflow counts for low-RPM accuracy
        fan_tach_period = (fan_tach_overflow_count * 65536UL) + capture - fan_tach_capture_last;

        fan_tach_overflow_count = 0;
        fan_tach_capture_last = capture;
        fan_tach_new_data = 1;
        fan_tach_last_tick = HAL_GetTick();
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