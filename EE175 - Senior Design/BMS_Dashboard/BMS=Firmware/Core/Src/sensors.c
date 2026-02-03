#include "sensors.h"
#include <stdio.h>
#include <string.h>
#include <math.h>


// Include your HAL header here when generating the real project
// #include "stm32f3xx_hal.h" 

/* -------------------------------------------------------------------------
   Private Variables & Drivers
   ------------------------------------------------------------------------- */

// Placeholder for the global state
volatile BMS_PackState_t bms_state;

// Placeholder for SPI Handle - in real code: extern SPI_HandleTypeDef hspi1;
// extern SPI_HandleTypeDef hspi1;

// Buffer for JSON transmission
static char msg_buffer[512];

/* -------------------------------------------------------------------------
   Private Function Prototypes
   ------------------------------------------------------------------------- */
static void SPI_Read_ADC_Frame(uint16_t *raw_buffer);
static float Convert_ADC_To_Voltage(uint16_t raw_counts);
static float Convert_Thermistor_To_Temp(uint16_t raw_counts);
void Sensors_JSON_Output(void);

/* -------------------------------------------------------------------------
   Public API Implementation
   ------------------------------------------------------------------------- */

void Sensors_SetELoad(bool enable, float current_mA) {
    bms_state.eload_enabled = enable;
    bms_state.eload_current_mA = current_mA;
    
    // Hardware Control (Mocked)
    // -------------------------------------------------------------------------
    // [SCHEMATIC INTEGRATION REQUIRED]
    // Signal: "E-LOAD_EN" (or similar from Block Diagram)
    // Action: Set GPIO Pin to Enable/Disable Load
    // TODO: User to Fill -> HAL_GPIO_WritePin(ELOAD_PORT, ELOAD_PIN, enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
    
    // Signal: "I_SET" (DAC Output)
    // Action: Set DAC Voltage to control Current Sink
    // TODO: User to Fill -> HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, (uint32_t)dac_val);
    
    if (enable) {
        // Mock indicator
        // HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    }
}

void Sensors_SetFan(bool auto_mode, uint8_t duty) {
    bms_state.fan_auto_mode = auto_mode;
    
    if (!auto_mode) {
        // Manual Mode
        bms_state.fan_pwm_duty = duty; 
        
        // [SCHEMATIC INTEGRATION REQUIRED]
        // Signal: "FAN-EN" or "FAN_PWM"
        // Action: Set Timer PWM Duty Cycle
        // TODO: User to Fill -> __HAL_TIM_SET_COMPARE(&htimX, TIM_CHANNEL_Y, duty_scaled);
    } else {
        // Auto Mode logic
    }
}
void Sensors_Init(void) {
    // 1. Zero out the state
    memset((void*)&bms_state, 0, sizeof(BMS_PackState_t));
    
    // 2. Initialize Low-Level Pins (CS lines high) if not done by HAL_MspInit
    // HAL_GPIO_WritePin(ADC_CS_GPIO_Port, ADC_CS_Pin, GPIO_PIN_SET);
    
    // 3. Send initial config to ADC if required (Dummy read to wake up)
    // SPI_Transmit_Cmd(CMD_WAKEUP);
    
    bms_state.data_valid = false;
}

/**
 * @brief  Main acquisition task. call this at 10Hz (Timer ISR or Main Loop)
 * @note   This is the "Non-RTOS" deterministic signal flow.
 */
void Sensors_Update_10Hz(void) {
    uint16_t raw_adc_data[BMS_CELL_COUNT + BMS_THERMISTOR_COUNT];
    
    /* 1. Hardware Protect: Verify Clock/Power stability (Optional SW check) */
    
    /* 2. Acquire Raw Data (Blocking or DMA check) */
    // In a real DMA system, this function calculates *last* transfer's integrity
    // For simple bring-up, we do blocking SPI here.
    SPI_Read_ADC_Frame(raw_adc_data);

    /* 3. Convert & Populate "The Truth" */
    for(int i = 0; i < BMS_CELL_COUNT; i++) {
        bms_state.cell_voltages_mV[i] = Convert_ADC_To_Voltage(raw_adc_data[i]);
    }
    
    // Sum cells for Pack Voltage (or read separate HV divider)
    float pack_sum = 0.0f;
    for(int i=0; i<BMS_CELL_COUNT; i++) pack_sum += bms_state.cell_voltages_mV[i];
    bms_state.pack_voltage_mV = pack_sum;

    /* 4. Telemetry Metadata & Mock E-Load Data */
    bms_state.sample_counter++;
    bms_state.data_valid = true;

    // --- MOCK DATA GENERATION START ---
    // Simulate Input Voltage (Fluctuating around 24V)
    static float mock_time = 0;
    mock_time += 0.1f;
    bms_state.eload_voltage_mV = 24000.0f + (500.0f * sinf(mock_time * 0.5f)); // 23.5V - 24.5V

    // Simulate Actual Current
    if (bms_state.eload_enabled) {
        // Current ramps up to target with some noise
        float diff = bms_state.eload_current_mA - bms_state.eload_actual_current_mA;
        bms_state.eload_actual_current_mA += diff * 0.1f; // Simple low-pass filter / ramp
        // Add minimal noise
        bms_state.eload_actual_current_mA += (rand() % 20) - 10; 
    } else {
        bms_state.eload_actual_current_mA *= 0.8f; // Decays to 0
        if(bms_state.eload_actual_current_mA < 1.0f) bms_state.eload_actual_current_mA = 0;
    }

    // Simulate Fan RPM
    // Linearly map duty cycle (0-100) to RPM (0-6000)
    uint16_t target_rpm = bms_state.fan_pwm_duty * 60; 
    // Smooth transition
    int rpm_diff = (int)target_rpm - (int)bms_state.fan_rpm;
    bms_state.fan_rpm += rpm_diff / 10;
    // --- MOCK DATA GENERATION END ---
    
    // In real code: bms_state.last_update_tick = HAL_GetTick();
    
    /* 5. Telemetry Output (JSON) */
    // Note: In a real RTOS, this would be queued to a separate task.
    // For non-RTOS, we do it here (watch out for timing budget!)
    Sensors_JSON_Output();
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
    printf("%s", msg_buffer); 
    // CDC_Transmit_FS((uint8_t*)msg_buffer, strlen(msg_buffer));
}

/* -------------------------------------------------------------------------
   Private Helper Functions
   ------------------------------------------------------------------------- */

// STUB: Replace with actual SPI Logic
static void SPI_Read_ADC_Frame(uint16_t *raw_buffer) {
    // Toggle CS Low
    // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
    
    // Transmit Read Command & Receive Data
    // HAL_SPI_TransmitReceive(&hspi1, tx_buf, rx_buf, len, 10);
    
    // Toggle CS High
    // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    
    // MOCK DATA FOR BRINGUP VERIFICATION
    for(int i=0; i<BMS_CELL_COUNT; i++) {
        // Return ~3.7V in raw counts (assuming 12-bit 0-5V scaled)
        // This ensures your USB print logic has something to show!
        raw_buffer[i] = 3031; // random sane value
    }
}

static float Convert_ADC_To_Voltage(uint16_t raw_counts) {
    // V = (Raw / Max) * Vref * Divider
    // Simple linear scaling
    float voltage_at_pin = (raw_counts / 4096.0f) * (ADC_VREF_MV / 1000.0f); 
    return voltage_at_pin * V_DIVIDER_RATIO * 1000.0f; // Return mV
}

static float Convert_Thermistor_To_Temp(uint16_t raw_counts) {
    // Implement Steinhart-Hart or Lookup Table here
    return 25.0f; // Stub 25C
}
