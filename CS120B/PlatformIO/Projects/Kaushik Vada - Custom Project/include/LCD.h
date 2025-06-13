#ifndef LCD_H_
#define LCD_H_

#include <avr/io.h>
#include <util/delay.h>

#define DATA_BUS   PORTB
#define CTL_BUS    PORTD
#define DATA_DDR   DDRB
#define CTL_DDR    DDRD

#define LCD_D4     PB1  // Arduino Pin 9
#define LCD_D5     PB2  // Arduino Pin 10
#define LCD_D6     PB3  // Arduino Pin 11
#define LCD_D7     PB4  // Arduino Pin 12
#define LCD_EN     PB0  // Arduino Pin 8
#define LCD_RS     PD7  // Arduino Pin 7

#define LCD_CMD_CLEAR_DISPLAY             0x01
#define LCD_CMD_CURSOR_HOME               0x02
#define LCD_CMD_DISPLAY_OFF               0x08
#define LCD_CMD_DISPLAY_NO_CURSOR         0x0c
#define LCD_CMD_DISPLAY_CURSOR_NO_BLINK   0x0E
#define LCD_CMD_DISPLAY_CURSOR_BLINK      0x0F
#define LCD_CMD_4BIT_2ROW_5X7             0x28
#define LCD_CMD_8BIT_2ROW_5X7             0x38 

void lcd_send_4bits(uint8_t nibble)
{
    // Clear D4-D7
    PORTB &= ~((1 << LCD_D4) | (1 << LCD_D5) | (1 << LCD_D6) | (1 << LCD_D7));

    if (nibble & 0x01) PORTB |= (1 << LCD_D4);
    if (nibble & 0x02) PORTB |= (1 << LCD_D5);
    if (nibble & 0x04) PORTB |= (1 << LCD_D6);
    if (nibble & 0x08) PORTB |= (1 << LCD_D7);

    PORTB |= (1 << LCD_EN);
    _delay_us(1);
    PORTB &= ~(1 << LCD_EN);
    _delay_us(100);
}

void lcd_send_command(uint8_t command)
{
    PORTD &= ~(1 << LCD_RS);  // RS = 0 for command
    lcd_send_4bits(command >> 4);
    lcd_send_4bits(command & 0x0F);
}

void lcd_write_character(char character)
{
    PORTD |= (1 << LCD_RS);  // RS = 1 for data
    lcd_send_4bits(character >> 4);
    lcd_send_4bits(character & 0x0F);
}

// Custom Power Button Symbol (5x8)
uint8_t power_symbol[8] = {
    0b00100, //   *
    0b01110, //  ***
    0b01110, //  ***
    0b01110, //  ***
    0b00100, //   *
    0b00000, //    
    0b00100, //   *
    0b00000  //    
};

// Function to create custom characters
void lcd_create_custom_chars() {
    lcd_send_command(0x40); // CGRAM address for custom char slot 0
    for (int i = 0; i < 8; i++) {
        lcd_write_character(power_symbol[i]);
    }
}

void lcd_clear();
void lcd_goto_xy(uint8_t line, uint8_t pos);

// Function for slide-in animation
void lcd_display_slide_animation(const char* str, uint8_t row, uint16_t delay_ms) {
    lcd_clear();
    int len = 0;
    while (str[len] != '\0') len++;

    for (int i = 0; i <= len; i++) {
        lcd_goto_xy(row, 0);
        for (int j = 0; j < i; j++) {
            lcd_write_character(str[j]);
        }
        _delay_ms(delay_ms);
    }

    // Add a delay to allow the user to see the final message
    _delay_ms(1000);
}

void lcd_init(void)
{
    DATA_DDR |= (1 << LCD_D4) | (1 << LCD_D5) | (1 << LCD_D6) | (1 << LCD_D7) | (1 << LCD_EN);
    CTL_DDR  |= (1 << LCD_RS);

    _delay_ms(40);
    lcd_send_4bits(0x03);
    _delay_ms(5);
    lcd_send_4bits(0x03);
    _delay_us(150);
    lcd_send_4bits(0x03);
    lcd_send_4bits(0x02);

    lcd_send_command(LCD_CMD_4BIT_2ROW_5X7);
    lcd_send_command(LCD_CMD_DISPLAY_CURSOR_BLINK);
    lcd_send_command(LCD_CMD_CLEAR_DISPLAY);
    _delay_ms(5);

    lcd_create_custom_chars(); // Initialize custom characters
}

void lcd_write_str(const char* str)
{
    int i = 0;
    while (str[i] != '\0')
    {
        lcd_write_character(str[i]);
        i++;
    }
}

void lcd_clear()
{
    lcd_send_command(LCD_CMD_CLEAR_DISPLAY);
    _delay_ms(5);
}

void lcd_goto_xy(uint8_t line, uint8_t pos)
{
    lcd_send_command((0x80 | (line << 6)) + pos);
    _delay_us(50);
}

#endif /* LCD_H_ */
