#include "sensors.h"
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

    /* 4. Telemetry Metadata */
    bms_state.sample_counter++;
    bms_state.data_valid = true;
    
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
    // Mocking Fan RPMs for now as they aren't in the struct yet
    // "i" = Pack Current (Amps)
    offset += snprintf(msg_buffer + offset, sizeof(msg_buffer)-offset, 
                       "],\"i\":%.2f,\"fan\":[%d,%d]}\r\n", 
                       bms_state.pack_current_mA / 1000.0f, 
                       1200, 1200); 

    // 5. Transmit
    // Weak stub - USER must retarget this to CDC_Transmit_FS
    // printf("%s", msg_buffer); 
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
