#ifndef INC_MCP4725_H_
#define INC_MCP4725_H_

#include "stm32f4xx_hal.h"

// MCP4725 I2C Address (Default is 0x60 or 0x61, check datasheet/hardware)
// Shifted left by 1 for HAL I2C (0x60 << 1 = 0xC0)
#define MCP4725_ADDR 		(0x60 << 1) 

// Register Commands
#define MCP4725_CMD_WRITE_DAC           0x40  // Write to DAC Register
#define MCP4725_CMD_WRITE_DAC_EEPROM    0x60  // Write to DAC Register & EEPROM

/**
 * @brief Initialize the MCP4725 driver
 * @param phi2c Pointer to I2C handle
 */
void MCP4725_Init(I2C_HandleTypeDef *phi2c);

/**
 * @brief Set the DAC output voltage
 * @param value 12-bit value (0-4095)
 * @param save_eeprom If true, saves value to EEPROM (power-on default)
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef MCP4725_SetValue(uint16_t value, uint8_t save_eeprom);

#endif /* INC_MCP4725_H_ */
