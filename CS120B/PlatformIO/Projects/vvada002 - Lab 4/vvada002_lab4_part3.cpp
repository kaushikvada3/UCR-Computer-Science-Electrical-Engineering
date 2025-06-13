/*        Your Name & E-mail: Kaushik Vada vvada002@ucr.edu

*          Discussion Section: 021

 *         Assignment: Lab #4  Exercise #3

 *         Exercise Description: [optional - include for your own benefit]

 *        

 *         I acknowledge all content contained herein, excluding template or example code, is my own original work.

 * 

 *         Demo Link: 

 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "timerISR.h"
#include "serialATmega.h"
//TODO: Copy and paste your nums[] and outNum[] from the previous lab. The wiring is still the same, so it should work instantly
int nums[17] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111, // 9
    0b01110111, // a
    0b01111100, // b
    0b00111001, // c
    0b01011110, // d
    0b01111001, // e
    0b01110001,  // f
    0x00        // off
};
void outNum(int num){
    PORTB = 0b00111111 & num; // depends on your wiring. Assign bits (f-a), which are bits (4-0) from nums[] to register PORTB
    PORTD = 0b10000000 & (num << 1); // Assign bits (g) which are bits 6 & 5 of nums[] to register PORTD
}
void ADC_init() {
    ADMUX = 0b01000000; // AVCC reference
    ADCSRA = 0b10000111; // Enable ADC, prescaler 128
}
unsigned int ADC_read(unsigned char chnl) {
    ADMUX = (ADMUX & 0xF0) | (chnl & 0x0F); // select ADC channel
    ADCSRA |= (1 << ADSC); // start conversion
    while (ADCSRA & (1 << ADSC)); // wait for conversion to finish
    uint8_t low = ADCL;
    uint8_t high = ADCH;
    return (high << 8) | low;
}
// Provided map()
long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
unsigned char value;
unsigned char cnt = 0;
unsigned int lightVal;
enum states {start, LED, snooze, danger_on, danger_off} state; //TODO: finish the enum for the SM
void Tick() {
//TODO: declare your static variables here or declare it globally
// State Transistions
//TODO: complete transitions
    switch (state) {
        case start: 
            state = LED;  
            break;
        case LED:
            PORTD = PORTD & 0xE0;
            if(PINC >> 2 & 0x01){
                state = snooze;
            }
            if(lightVal < 50 && value > 3){
                state = danger_on;
            }
            break;
        case snooze:
            if(cnt < 6){
                cnt++;
            }
            else{
                state = LED;
                cnt = 0;
            }
            break;
        case danger_on:
            PORTD = PORTD | 0xA0;
            if(lightVal >= 50 || value < 3){
                state = LED;
            }
            else{
              state = danger_off;
            }
            break;
        case danger_off:
            PORTD = PORTD & 0xE0;
            if(lightVal >= 50 || value < 3){
                state = LED;
            }
            else{
              state = danger_on;
            }
            break;

    }
    // State Actions
    //TODO: complete transitions
    switch (state) {
        case start:
          break;
        case LED:
            TimerSet(500);
            outNum(nums[value]);
            if(value <= 3){
                PORTD = PORTD | 0x44;
            }
            if(value >= 4 && value < 8){
                PORTD = PORTD | 0x58;
            }
            if(value >= 8){
                PORTD = PORTD | 0x50;
            }
            break;
        case snooze:
            outNum(0b01000000);
            PORTD = PORTD | 0x40;
            break;
        case danger_on:
            outNum(nums[13]); // Show 'd' for danger
            /* Enable only D1 (PD5 high), disable D4 (PD6 low) */
            PORTD = (PORTD & ~((1<<PD5) | (1<<PD6))) | (1<<PD5);
            /* Turn RED LED on (PD4 high, common‑cathode) */
            PORTD |= (1<<PD4);
            break;
        case danger_off:
            outNum(nums[16]); // Blank/off digit
            /* Keep only D1 active */
            PORTD = (PORTD & ~((1<<PD5) | (1<<PD6))) | (1<<PD5);
            /* Keep RED LED on continuously */
            PORTD |= (1<<PD4);
            break;
    }
}
int main(void)
{
    //TODO: initialize all outputs and inputs
    DDRC = 0x00;
    PORTC = 0xFF;
    DDRB = 0xFF;
    PORTB = 0x00;
    DDRD = 0xFF;
    PORTD = 0x00;
    ADC_init();
    serial_init(9600);
    TimerSet(500);
    TimerOn();
    state = start;
    while (1){
        lightVal = ADC_read(0); // Read photoresistor value from A0
        value = map(ADC_read(1), 15, 1023, 0, 9);
        serial_println("Light ADC:");
        serial_println(lightVal);
        Tick(); // Execute one synchSM tick
        while (!TimerFlag){} // Wait for SM period
        TimerFlag = 0;
    }
    return 0;
}
