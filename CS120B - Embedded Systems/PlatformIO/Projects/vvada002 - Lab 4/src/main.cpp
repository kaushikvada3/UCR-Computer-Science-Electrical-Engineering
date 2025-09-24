/* Kaushik Vada - vvada002@ucr.edu
 * Discussion: 021
 * Assignment: Lab 4, Exercise 3
 * 
 * This code is original work by the author listed above.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "timerISR.h"
#include "serialATmega.h"

// Hex values for 7-segment display
int nums[16] = {
    0b00111111,  // 0
    0b00000110,  // 1
    0b01011011,  // 2
    0b01001111,  // 3
    0b01100110,  // 4
    0b01101101,  // 5
    0b01111101,  // 6
    0b00000111,  // 7
    0b01111111,  // 8
    0b01101111,  // 9
    0b01110111,  // A
    0b01111100,  // b
    0b00111001,  // C
    0b01011110,  // d
    0b01111001,  // E
    0b01110001   // F
};

// Output digit to 7-segment display
void outNum(int num) {
    PORTB = num & 0b00111111;               // bits a-f
    PORTD = (num << 1) & 0b10000000;        // bit g
}

// ADC setup
void ADC_init() {
    ADMUX  = 0b01000000;      // AVCC reference
    ADCSRA = 0b10000111;      // Enable ADC, start conversion, prescaler set
    ADCSRB = 0b01000000;      // ACME = 1
}

// Read ADC from channel
unsigned int ADC_read(unsigned char chnl) {
    ADMUX |= chnl;
    ADCSRA |= 0b01000000;         // Start conversion
    while (ADCSRA & (1 << 6)) {}  // Wait until conversion completes
    uint8_t low = ADCL;
    uint8_t high = ADCH;
    return (high << 8) | low;
}

// Map function
long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

unsigned char value;
unsigned char cnt = 0;

// State machine states
enum states { start, LED, snooze } state;

void Tick() {
    // State transitions
    switch (state) {
        case start:
            state = LED;
            break;

        case LED:
            PORTD &= 0xE0;
            if (PINC & (1 << 2)) {
                state = snooze;
            }
            break;

        case snooze:
            if (cnt < 6) {
                cnt++;
            } else {
                state = LED;
                cnt = 0;
            }
            break;
    }

    // State actions
    switch (state) {
        case start:
            break;

        case LED:
            outNum(nums[value]);
            if (value <= 3) {
                PORTD |= 0x44;
            } else if (value < 8) {
                PORTD |= 0x58;
            } else {
                PORTD |= 0x50;
            }
            break;

        case snooze:
            outNum(0b01000000);
            PORTD |= 0x40;
            break;
    }
}

int main(void) {
    DDRC = 0x00;  PORTC = 0xFF;
    DDRB = 0xFF;  PORTB = 0x00;
    DDRD = 0xFF;  PORTD = 0x00;

    ADC_init();
    serial_init(9600);
    TimerSet(500);
    TimerOn();

    state = start;

    while (1) {
        value = map(ADC_read(1), 15, 1023, 0, 9);
        Tick();
        while (!TimerFlag) {}
        TimerFlag = 0;
    }

    return 0;
}