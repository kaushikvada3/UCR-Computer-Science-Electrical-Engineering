/*        Your Name & E-mail: Kaushik Vada vvada002@ucr.edu

*          Discussion Section: 021

 *         Assignment: Lab #6  Exercise #1

 *         Exercise Description: [optional - include for your own benefit]

 *        

 *         I acknowledge all content contained herein, excluding template or example code, is my own original work.

 *

 *         Demo Link: 
 */
#include <avr/interrupt.h>
#include <avr/io.h>
#include "timerISR.h"

// 8 phases of the stepper motor step
int phases[8] = {0b0001, 0b0011, 0b0010, 0b0110, 0b0100, 0b1100, 0b1000, 0b1001};

// Variables for cross-task communication
unsigned short joystickX = 0; // Stores X-axis value from joystick
unsigned short joystickY = 0; // Stores Y-axis value from joystick
unsigned char ledState = 0;   // Tracks the LED pattern state

int nums[17] = {0b1111110, 0b0110000, 0b1101101, 0b1111001, 0b0110011, 0b1011011, 0b1011111, 0b1110000, 0b1111111, 0b1111011, 0b1110111, 0b0011111, 0b1001110, 0b0111101, 0b1001111, 0b1000111,0b0000001}; 
// a  b  c  d  e  f  g

// Sets or clears a bit in a byte
unsigned char SetBit(unsigned char x, unsigned char k, unsigned char b) {
    return (b ?  (x | (0x01 << k))  :  (x & ~(0x01 << k)) );
    //   Set bit to 1           Set bit to 0
}

// Output a number to the 7-segment display
void outNum(int num){
    PORTD = num << 1;
    PORTB = SetBit(PORTB, 0, num & 0x01);
}
// Initialize the ADC (Analog to Digital Converter)
void ADC_init() {
    ADMUX = 0b01000000; // bit 7:6 -> 0:1 (AVCC ref volt)
    ADCSRA = 0b10000111; // bit7=1 -> ADC enable, bit6->conversion, bit5->(auto trigger), bit2-0 -> prescaler
    ADCSRB = 0b01000000; // bits0-2 decide mode and the 6th bit is ACME (not used here)
}

// Read from the ADC channel
unsigned int ADC_read(unsigned char chnl){
    ADMUX &= 0xF0;
    ADMUX = ADMUX | chnl; // select the output channel
    ADCSRA = ADCSRA | 0b01000000;
    while((ADCSRA >> 6) & 0x01) {} // wait for ADSC to clear
    uint8_t low, high;
    low = ADCL;
    high = ADCH;
    return ((high << 8) | low);
}

// Number of concurrent tasks
#define NUM_TASKS 2

// Task struct for concurrent state machines
typedef struct _task {
    signed char state; // Task's current state
    unsigned long period; // Task period
    unsigned long elapsedTime; // Time elapsed since last task tick
    int (*TickFct)(int); // Task tick function
} task;

// Task periods
const unsigned long JS_PERIOD = 200;
const unsigned long LED_PERIOD = 200;
const unsigned long GCD_PERIOD = 100; // Calculate GCD of tasks

task tasks[NUM_TASKS]; // Task array

// Joystick State Machine States
enum JS_States {JS_Start, JS_read};

// Joystick State Machine Tick Function
// Reads joystick values and displays direction on 7-segment
int JS_Tick(int state) {
    switch(state) {
        case JS_Start:
            state = JS_read;
            break;
        case JS_read:
            state = JS_read;
            break;
    }
    switch(state) {
        case JS_Start:
            break;
        case JS_read:
            joystickX = ADC_read(3);
            joystickY = ADC_read(2);
            if(joystickX < 400) { // left
                outNum(0b0001110);
            }
            else if(joystickX > 600) { // right
                outNum(0b0000101);
            }
            else if(joystickY > 20) { // up
                outNum(0b0111110);
            }
            else if(joystickY < 1) { // down
                outNum(0b0111101);
            }
            else {
                outNum(0b0000001);
            }
            break;
    }
    return state;
}

// LED State Machine States
enum LED_States {LED_start, LED_read};

// LED State Machine Tick Function
// Handles button input and updates LED pattern state
int LED_Tick(int state) {
    switch(state) {
        case LED_start:
            state = LED_read;
            break;
        case LED_read:
            if(!(PINC >> 4 & 0x01)) {
                if(ledState < 3) {
                    ledState++;
                }
                else {
                    ledState = 0;
                }
            }
            break;
    }
    switch(state) {
        case LED_start:
            break;
        case LED_read:
            PORTC &= 0xFC;
            PORTC |= ledState;
            break;
    }
    return state;
}


// Timer interrupt service routine: calls each task's tick function if ready
void TimerISR() {
    for (unsigned int i = 0; i < NUM_TASKS; i++) {
        if (tasks[i].elapsedTime == tasks[i].period) {
            tasks[i].state = tasks[i].TickFct(tasks[i].state);
            tasks[i].elapsedTime = 0;
        }
        tasks[i].elapsedTime += GCD_PERIOD;
    }
}

// Main function: initializes hardware and tasks, then runs scheduler
int main(void) {
    // Initialize all inputs and outputs
    DDRC = 0b00000111;
    PORTC = 0b00011000;
    DDRB = 0xFF;
    PORTB = 0x00;
    DDRD = 0xFF;
    PORTD = 0x00;

    ADC_init(); // initialize ADC

    // Initialize tasks
    tasks[0].period = JS_PERIOD;
    tasks[0].state = JS_Start;
    tasks[0].elapsedTime = 0;
    tasks[0].TickFct = &JS_Tick;

    tasks[1].period = LED_PERIOD;
    tasks[1].state = LED_start;
    tasks[1].elapsedTime = 0;
    tasks[1].TickFct = &LED_Tick;

    TimerSet(GCD_PERIOD);
    TimerOn();

    while (1) {
        // Scheduler handled in TimerISR
    }
    return 0;
}