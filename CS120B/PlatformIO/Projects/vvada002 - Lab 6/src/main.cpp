/*        Your Name & E-mail: Kaushik Vada vvada002@ucr.edu

*          Discussion Section: 021

 *         Assignment: Lab #6  Exercise #3

 *         Exercise Description: [optional - include for your own benefit]

 *        

 *         I acknowledge all content contained herein, excluding template or example code, is my own original work.

 *

 *         Demo Link: 
 */
#include <avr/interrupt.h>
#include <avr/io.h>
#include "timerISR.h"


// ==========================================================
//                GLOBAL VARIABLES & CONSTANTS
// ==========================================================

int g_phases[8] = {0b0001, 0b0011, 0b0010, 0b0110, 0b0100, 0b1100, 0b1000, 0b1001}; // 8 phases of the stepper motor step

unsigned short g_joystick_x = 0;
unsigned short g_joystick_y = 0;
unsigned char g_move_index = 0;
bool g_should_rotate = false;
bool g_wrong = false;
bool g_is_unlocked = false;
unsigned char g_c = 0;
unsigned char g_p = 0;
unsigned char g_phase_index = 0;
unsigned short g_step_p = 0;
char g_user_input[4];
char g_passcode[4] = {'U','D','L','R'};
unsigned char g_movement_started = 0;
unsigned g_correct_count = 0;

// ==========================================================
//                UTILITY FUNCTIONS
// ==========================================================

void Empty_Array(char* arr, unsigned char size) {
    if (arr[0] == '\0') return;
    for (unsigned char i = 0; i < size; ++i) {
        arr[i] = '\0';
    }
}

void Change_Passcode(char* arr, unsigned char size) {
    if (arr[0] == '\0') return;
    for (unsigned char i = 0; i < size; ++i) {
        g_passcode[i] = arr[i];
    }
}

unsigned char Set_Bit(unsigned char x, unsigned char k, unsigned char b) {
    return (b ? (x | (0x01 << k)) : (x & ~(0x01 << k)));
    //   Set bit to 1           Set bit to 0
}

void Output_Number(int num) {
    PORTD = num << 1;
    PORTB = Set_Bit(PORTB, 0, num & 0x01);
}

// ==========================================================
//                ADC FUNCTIONS
// ==========================================================

void ADC_Init() {
    ADMUX = 0b01000000; //bit 7:6 -> 0:1 (AVCC ref volt)
    ADCSRA = 0b10000111; // bit7=1 -> ADC enable, bit6->conversion, bit5->(auto trigger), bit2-0 -> prescaler
    ADCSRB = 0b01000000; // bits0-2 decide mode and the 6th bit is ACME
                         // but ADEN in ADCSRA is already 0 so ACME is negligible
}

unsigned int ADC_Read(unsigned char chnl) {
    ADMUX &= 0xF0;
    ADMUX = ADMUX | chnl; //select the output chnl
    ADCSRA = ADCSRA | 0b01000000;
    while((ADCSRA >> 6) & 0x01) {} // wait for ADSC to clear
    uint8_t low, high;
    low = ADCL;
    high = ADCH;
    return ((high << 8) | low);
}

// ==========================================================
//                TASK STRUCTURES & CONSTANTS
// ==========================================================

#define NUM_TASKS 3

typedef struct _task {
    signed char state;
    unsigned long period;
    unsigned long elapsedTime;
    int (*TickFct)(int);
} task;

const unsigned long JOYSTICK_PERIOD = 200;
const unsigned long LED_PERIOD = 200;
const unsigned long STEPPER_PERIOD = 1;
const unsigned long GCD_PERIOD = 1;

task g_tasks[NUM_TASKS];

// ==========================================================
//                TASK STATE ENUMS
// ==========================================================

typedef enum {
    JOYSTICK_START,
    JOYSTICK_READ,
    JOYSTICK_UNLOCK,
    JOYSTICK_WRONG,
    JOYSTICK_RESET
} JOYSTICK_States;

typedef enum {
    LED_START,
    LED_READ,
    LED_CRACKED,
    LED_WRONG,
    LED_RESET
} LED_States;

typedef enum {
    STEPPER_START,
    STEPPER_STAND,
    STEPPER_TURN_CW,
    STEPPER_TURN_CCW
} STEPPER_States;

// ==========================================================
//                TASK: JOYSTICK
// ==========================================================

int Tick_Joystick(int state) {
    switch(state) {
        case JOYSTICK_START:
            state = JOYSTICK_READ;
            break;

        case JOYSTICK_READ:
            if (g_wrong) {
                state = JOYSTICK_WRONG;
            }

            else if (!(PINC >> 4 & 0x01) && g_is_unlocked) {
                Empty_Array(g_user_input, 4);
                g_move_index = 0;
                state = JOYSTICK_RESET;
            }
            break;

        case JOYSTICK_UNLOCK:
            if (!g_should_rotate) {
                state = JOYSTICK_READ;
            }
            break;

        case JOYSTICK_WRONG:
            if (!g_wrong) {
                state = JOYSTICK_READ;
            }
            break;

        case JOYSTICK_RESET:
            PORTB = Set_Bit(PORTB, 1, 1);
            break;
    }

    switch(state) {
        case JOYSTICK_START:
            break;

        case JOYSTICK_READ:
            g_joystick_x = ADC_Read(3);
            g_joystick_y = ADC_Read(2);

            if (g_joystick_x < 400) { // left
                Output_Number(0b0001110);
                g_movement_started = 1;
                g_user_input[g_move_index] = 'L';
            }
            else if (g_joystick_x > 600) { // right
                Output_Number(0b0000101);
                g_movement_started = 1;
                g_user_input[g_move_index] = 'R';
            }
            else if (g_joystick_y > 20) { // up
                Output_Number(0b0111110);
                g_movement_started = 1;
                g_user_input[g_move_index] = 'U';
            }
            else if (g_joystick_y < 1) { // down
                Output_Number(0b0111101);
                g_movement_started = 1;
                g_user_input[g_move_index] = 'D';
            }
            else {
                Output_Number(0b0000001);

                if (g_movement_started > 0) {
                    if (g_move_index >= 3) {
                        g_move_index = 0;

                        for (unsigned char i = 0; i < 4; i++) {
                            if (g_user_input[i] == g_passcode[i]) {
                                g_correct_count++;
                            }
                        }

                        if (g_correct_count == 4) {
                            g_should_rotate = true;
                            g_correct_count = 0;
                            state = JOYSTICK_UNLOCK;
                        }
                        else {
                            g_correct_count = 0;
                            g_wrong = true;
                        }

                        Empty_Array(g_user_input, 4);
                    }
                    else {
                        g_move_index++;
                    }
                    g_movement_started = 0;
                }
            }
            break;

        case JOYSTICK_UNLOCK:
            break;

        case JOYSTICK_RESET:
            g_joystick_x = ADC_Read(3);
            g_joystick_y = ADC_Read(2);

            if (g_joystick_x < 400) { // left
                Output_Number(0b0001110);
                g_movement_started = 1;
                g_user_input[g_move_index] = 'L';
            }
            else if (g_joystick_x > 600) { // right
                Output_Number(0b0000101);
                g_movement_started = 1;
                g_user_input[g_move_index] = 'R';
            }
            else if (g_joystick_y > 20) { // up
                Output_Number(0b0111110);
                g_movement_started = 1;
                g_user_input[g_move_index] = 'U';
            }
            else if (g_joystick_y < 1) { // down
                Output_Number(0b0111101);
                g_movement_started = 1;
                g_user_input[g_move_index] = 'D';
            }
            else {
                Output_Number(0b0000001);

                if (g_movement_started > 0) {
                    if (g_move_index > 2) {
                        g_move_index = 0;
                        Change_Passcode(g_user_input, 4);
                        PORTB = Set_Bit(PORTB, 1, 0);
                        state = JOYSTICK_READ;
                    }
                    else {
                        g_move_index++;
                    }
                    g_movement_started = 0;
                }
            }
            break;
    }
    return state;
}

// ==========================================================
//                TASK: LED
// ==========================================================

int Tick_LED(int state) {
    switch(state) {
        case LED_START:
            state = LED_READ;
            break;

        case LED_READ:
            if (g_should_rotate) {
                state = LED_CRACKED;
            }

            else if (g_wrong) {
                state = LED_WRONG;
            }
            break;

        case LED_CRACKED:
            if (!g_should_rotate) {
                PORTC &= 0xFC;
                state = LED_READ;
            }
            break;

        case LED_WRONG:
            if (g_c >= 40) {
                g_c = 0;
                g_p = 0;
                g_wrong = false;
                state = LED_READ;
            }
            g_p++;
            g_c++;
            break;
    }

    switch(state) {
        case LED_START:
            break;

        case LED_READ:
            PORTC &= 0xFC;
            PORTC |= g_move_index;
            break;

        case LED_CRACKED:
            PORTC |= 3;
            break;

        case LED_WRONG:
            if (g_p < 5) {
                PORTC |= 0x03;
            }
            else if (g_p >= 5) {
                PORTC &= 0xFC;
                g_p = 0;
            }
            break;

        case LED_RESET:
            break;
    }
    return state;
}

// ==========================================================
//                TASK: STEPPER MOTOR
// ==========================================================

int Tick_Stepper(int state) {
    switch(state) {
        case STEPPER_START:
            state = STEPPER_STAND;
            break;

        case STEPPER_STAND:
            if (g_should_rotate) {
                PORTB = (PORTB & 0x03) | g_phases[g_phase_index] << 2;
                state = STEPPER_TURN_CW;
            }
            break;

        case STEPPER_TURN_CW:
            if (g_is_unlocked) {
                state = STEPPER_TURN_CCW;
            }
            break;

        case STEPPER_TURN_CCW:
            if (!g_is_unlocked) {
                state = STEPPER_TURN_CCW;
            }
            break;
    }

    switch(state) {
        case STEPPER_START:
            break;

        case STEPPER_STAND:
            if (g_should_rotate) {
                state = STEPPER_TURN_CW;
            }
            break;

        case STEPPER_TURN_CW:
            if (g_step_p < 900) {
                if (g_phase_index < 8) {
                    PORTB = (PORTB & 0x03) | g_phases[g_phase_index] << 2;
                }
                else {
                    g_phase_index = 0;
                }
                g_phase_index++;
            }
            else {
                g_step_p = 0;
                g_phase_index = 7;
                g_should_rotate = false;
                g_is_unlocked = !g_is_unlocked;
                state = STEPPER_STAND;
            }
            g_step_p++;
            break;

        case STEPPER_TURN_CCW:
            if (g_step_p < 900) {
                g_phase_index--;
                if (g_phase_index > 0) {
                    PORTB = (PORTB & 0x03) | g_phases[g_phase_index] << 2;
                }
                else if (g_phase_index == 0) {
                    PORTB = (PORTB & 0x03) | g_phases[g_phase_index] << 2;
                    g_phase_index = 7;
                }
            }
            else {
                g_step_p = 0;
                g_phase_index = 0;
                g_should_rotate = false;
                g_is_unlocked = !g_is_unlocked;
                state = STEPPER_STAND;
            }
            g_step_p++;
            break;
    }
    return state;
}


// ==========================================================
//                TIMER ISR
// ==========================================================

void TimerISR() {
    for (unsigned int i = 0; i < NUM_TASKS; i++) {
        if (g_tasks[i].elapsedTime == g_tasks[i].period) {
            g_tasks[i].state = g_tasks[i].TickFct(g_tasks[i].state);
            g_tasks[i].elapsedTime = 0;
        }
        g_tasks[i].elapsedTime += GCD_PERIOD;
    }
}

// ==========================================================
//                MAIN FUNCTION
// ==========================================================

int main(void) {
    // --------------------
    // PORT INITIALIZATION
    // --------------------
    DDRC =  0b00000111;
    PORTC = 0b00011000;
    DDRB = 0xFF;
    PORTB = 0x00;
    DDRD = 0xFF;
    PORTD = 0x00;

    // --------------------
    // ADC INITIALIZATION
    // --------------------
    ADC_Init();

    // --------------------
    // TASK INITIALIZATION
    // --------------------
    g_tasks[0].period = JOYSTICK_PERIOD;
    g_tasks[0].state = JOYSTICK_START;
    g_tasks[0].elapsedTime = 0;
    g_tasks[0].TickFct = &Tick_Joystick;

    g_tasks[1].period = LED_PERIOD;
    g_tasks[1].state = LED_START;
    g_tasks[1].elapsedTime = 0;
    g_tasks[1].TickFct = &Tick_LED;

    g_tasks[2].period = STEPPER_PERIOD;
    g_tasks[2].state = STEPPER_START;
    g_tasks[2].elapsedTime = 0;
    g_tasks[2].TickFct = &Tick_Stepper;

    // --------------------
    // TIMER START
    // --------------------
    TimerSet(GCD_PERIOD);
    TimerOn();

    while (1) {}
    return 0;
}
