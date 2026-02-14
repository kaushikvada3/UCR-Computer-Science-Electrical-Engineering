#include "sensors.h"
#include "main.h"
/* #include "stm32f4xx_hal.h" -- Removed, using main.h */
#include "mcp4725.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* -------------------------------------------------------------------------
   External Handles
   ------------------------------------------------------------------------- */
extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c1;
extern TIM_HandleTypeDef htim4; // Changed to TIM4

/* -------------------------------------------------------------------------
   Private Variables & Constants
   ------------------------------------------------------------------------- */
volatile BMS_PackState_t bms_state;
static char msg_buffer[512];

// Safety Thresholds
#define MAX_CURRENT_MA          3000.0f  // 3A Limit
#define MAX_VOLTAGE_MV          25000.0f // 25V Limit
#define MIN_VOLTAGE_MV          3000.0f  // 3V UVLO

// Hardware Constants
#define SHUNT_RESISTANCE        0.01f    // 10mOhm Shunt
#define OPAMP_GAIN              50.0f    // Current Sense OpAmp Gain

/* -------------------------------------------------------------------------
   Private Function Prototypes
   ------------------------------------------------------------------------- */
static void Check_Safety_FSM(void);

/* -------------------------------------------------------------------------
   Public API Implementation
   ------------------------------------------------------------------------- */

void Sensors_Init(void) {
    // 1. Zero out the state
    memset((void*)&bms_state, 0, sizeof(BMS_PackState_t));
    
    // 2. Init Drivers
    MCP4725_Init(&hi2c1);
    
    // 3. Start Peripherals
    HAL_ADC_Start(&hadc1);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1); // TIM4 PB6
    
    // 4. Default Safe State
    // Hardware: GATE_KILL (PB0) must be HIGH to DISABLE.
    // HAL_Init in main.c already sets it HIGH.
    Sensors_SetELoad(false, 0.0f);
    Sensors_SetFan(true, 0); // Auto mode
    
    bms_state.data_valid = true;
}

void Sensors_SetELoad(bool enable, float current_mA) {
    bms_state.eload_enabled = enable;
    bms_state.eload_current_mA = current_mA;
    
    if (enable) {
        // 1. Set DAC First
        float dac_voltage = (current_mA / 10000.0f) * 3.3f; 
        uint16_t dac_val = (uint16_t)((dac_voltage / 3.3f) * 4095.0f);
        MCP4725_SetValue(dac_val, 0);
        
        // 2. ENABLE Gate Drive (Active LOW)
        // PB0 = LOW to TURN ON
        #ifdef GATE_DISABLE_Pin
        HAL_GPIO_WritePin(GATE_DISABLE_Port, GATE_DISABLE_Pin, GPIO_PIN_RESET); 
        #endif
    } else {
        // 1. DISABLE Gate Drive (Active HIGH Kill)
        // PB0 = HIGH to KILL
        #ifdef GATE_DISABLE_Pin
        HAL_GPIO_WritePin(GATE_DISABLE_Port, GATE_DISABLE_Pin, GPIO_PIN_SET);
        #endif
        
        // 2. Set DAC to 0
        MCP4725_SetValue(0, 0);
    }
}

void Sensors_SetFan(bool auto_mode, uint8_t duty) {
    bms_state.fan_auto_mode = auto_mode;
    
    if (!auto_mode) {
        bms_state.fan_pwm_duty = duty;
        // Scale 0-100 to 0-1000 (Timer Period)
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, duty * 10);
    }
}

void Sensors_Update_10Hz(void) {
    // 1. Read System Voltage
    HAL_ADC_PollForConversion(&hadc1, 10);
    uint16_t adc_vsense = HAL_ADC_GetValue(&hadc1);
    
    // V_IN = (ADC / 4095) * 3.3V * Divider
    // Divider = 11.0
    float vin_v = (adc_vsense / 4095.0f) * 3.3f * V_DIVIDER_RATIO;
    bms_state.eload_voltage_mV = vin_v * 1000.0f;
    
    // 2. Read Current
    // Placeholder: In real logic, switch channel or read 2nd Rank
    uint16_t adc_ishunt = 0; 
    
    // I = V_ADC / (R * G)
    float v_shunt_v = (adc_ishunt / 4095.0f) * 3.3f;
    float current_a = v_shunt_v / (SHUNT_RESISTANCE * OPAMP_GAIN);
    bms_state.eload_actual_current_mA = current_a * 1000.0f;
    
    // 3. Safety Check
    Check_Safety_FSM();
    
    // 4. GUI Telemetry Mapping
    bms_state.pack_current_mA = bms_state.eload_actual_current_mA;
    bms_state.pack_voltage_mV = bms_state.eload_voltage_mV;
    
    float avg_cell = bms_state.pack_voltage_mV / 12.0f;
    for(int i=0; i<BMS_CELL_COUNT; i++) {
        bms_state.cell_voltages_mV[i] = avg_cell;
    }

    // 5. Updates
    bms_state.sample_counter++;
    Sensors_JSON_Output();
}

static void Check_Safety_FSM(void) {
    // Software Over-Current
    if (bms_state.eload_actual_current_mA > MAX_CURRENT_MA) {
        bms_state.error_flags |= 0x02;
    }
    
    // Software Over-Voltage
    if (bms_state.eload_voltage_mV > MAX_VOLTAGE_MV) {
        bms_state.error_flags |= 0x04;
    }
    
    // Trip Logic
    if (bms_state.error_flags != 0) {
        Sensors_SetELoad(false, 0); // Safety Kill (PB0 HIGH)
    }
}

/**
 * @brief  Serializes Current State to JSON and prints it.
 * @note   Schema: {"v":[c1,c2...], "t":[t1,t2...], "i":current, "fan":[f1,f2]}
 */
void Sensors_JSON_Output(void) {
    if(!bms_state.data_valid) return;

    // 1. Start Object
    int offset = snprintf(msg_buffer, sizeof(msg_buffer), "{\"v\":[");

    // 2. Cell Voltages (Array)
    for(int i=0; i<BMS_CELL_COUNT; i++) {
        offset += snprintf(msg_buffer + offset, sizeof(msg_buffer)-offset, 
                           "%.2f%s", 
                           bms_state.cell_voltages_mV[i] / 1000.0f, // Convert mV to V for GUI
                           (i < BMS_CELL_COUNT-1) ? "," : "");
    }

    // 3. Temperatures (Array)
    offset += snprintf(msg_buffer + offset, sizeof(msg_buffer)-offset, "],\"t\":[");
    for(int i=0; i<BMS_THERMISTOR_COUNT; i++) {
        offset += snprintf(msg_buffer + offset, sizeof(msg_buffer)-offset, 
                           "%.1f%s", 
                           bms_state.temperatures_C[i], 
                           (i < BMS_THERMISTOR_COUNT-1) ? "," : "");
    }

    // 4. Current & Fans
    // "i" = Pack Current (Amps)
    offset += snprintf(msg_buffer + offset, sizeof(msg_buffer)-offset, 
                       "],\"i\":%.2f,", 
                       bms_state.pack_current_mA / 1000.0f);

    // 5. Fan Control & RPM
    offset += snprintf(msg_buffer + offset, sizeof(msg_buffer)-offset, 
                       "\"fan_ctrl\":{\"auto\":%d,\"duty\":%d,\"rpm\":%d},",
                       bms_state.fan_auto_mode ? 1 : 0,
                       bms_state.fan_pwm_duty,
                       bms_state.fan_rpm);

    // 6. E-Load Stats (Target vs Actual)
    // "en": Enable State
    // "i_set": Target Current (A)
    // "v": Input Voltage (V)
    // "i_act": Actual Current (A)
    // "p": Power (W) = V * I_act
    float power_W = (bms_state.eload_voltage_mV / 1000.0f) * (bms_state.eload_actual_current_mA / 1000.0f);
    
    offset += snprintf(msg_buffer + offset, sizeof(msg_buffer)-offset, 
                       "\"eload_stats\":{\"en\":%d,\"i_set\":%.3f,\"v\":%.2f,\"i_act\":%.3f,\"p\":%.2f}}\r\n", 
                       bms_state.eload_enabled ? 1 : 0,
                       bms_state.eload_current_mA / 1000.0f,
                       bms_state.eload_voltage_mV / 1000.0f,
                       bms_state.eload_actual_current_mA / 1000.0f,
                       power_W);

    // 7. Transmit
    // Use printf, which is redirected to USB CDC in main.c via _write()
    // printf("%s", msg_buffer); 
    
    // Direct USB Transmission (More robust than printf hook)
    extern uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);
    CDC_Transmit_FS((uint8_t*)msg_buffer, strlen(msg_buffer));
}

/* -------------------------------------------------------------------------
   Private Helper Functions
   ------------------------------------------------------------------------- */
// (Empty - Helpers integrated or removed)
