#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>
#include <stdbool.h>

/* Application Configuration */
#define BMS_CELL_COUNT          10      // 10s1p Battery Pack
#define BMS_THERMISTOR_COUNT    10      // 10 NTCs (1 per cell)
#define SENSOR_UPDATE_RATE_HZ   10      // 10Hz Sampling

/* Hardware Definitions - Adapt these to your ADC */
#define ADC_VREF_MV             3300.0f
#define ADC_RESOLUTION_BITS     12      // Native MCU ADC or External
#define V_DIVIDER_RATIO         11.0f   // Example: 100k over 10k

/* Main Data Structure - The "Truth" */
typedef struct {
    /* Critical Analog Values */
    float cell_voltages_mV[BMS_CELL_COUNT]; // Individual cell voltages
    float pack_voltage_mV;                 // Sum of cells or Pack+ pin
    float pack_current_mA;                 // Instantaneous current
    float temperatures_C[BMS_THERMISTOR_COUNT]; 

    /* System Status */
    uint32_t sample_counter;
    uint32_t last_update_tick;
    bool     data_valid;
    
    /* Flags (Populated by Safety Layer later, but defined here) */
    uint32_t error_flags;
} BMS_PackState_t;

/* Global Instance - External access for read-only via getters is preferred, 
   but direct access is fine for simple non-RTOS skeleton */
extern volatile BMS_PackState_t bms_state;

/* Public API */
void Sensors_Init(void);
void Sensors_Update_10Hz(void);

#endif // SENSORS_H
