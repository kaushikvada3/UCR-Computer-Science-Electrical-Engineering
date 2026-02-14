#include "mcp4725.h"

static I2C_HandleTypeDef *_phi2c;

void MCP4725_Init(I2C_HandleTypeDef *phi2c) {
    _phi2c = phi2c;
}

HAL_StatusTypeDef MCP4725_SetValue(uint16_t value, uint8_t save_eeprom) {
    uint8_t packet[3];
    
    if(value > 4095) value = 4095;

    // Byte 0: Command
    packet[0] = save_eeprom ? MCP4725_CMD_WRITE_DAC_EEPROM : MCP4725_CMD_WRITE_DAC;
    
    // Byte 1: Data MSB (D11-D4)
    packet[1] = (value >> 4);
    
    // Byte 2: Data LSB (D3-D0) << 4
    packet[2] = (value & 0x0F) << 4;

    return HAL_I2C_Master_Transmit(_phi2c, MCP4725_ADDR, packet, 3, 100);
}
