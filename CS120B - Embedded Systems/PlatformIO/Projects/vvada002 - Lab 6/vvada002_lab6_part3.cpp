#include <avr/io.h> // AVR I/O library for port and pin manipulation
#include <avr/interrupt.h> // AVR interrupt library
#include <util/delay.h> // AVR delay library for timing
#include <stdlib.h> // Standard library for random number generation
#include <stdio.h> // Standard I/O library for debugging
#include "LCD.h" // Custom library for LCD control
#include "timerISR.h" // Custom library for timer interrupt service routines
#include "periph.h" // Custom library for peripheral initialization (e.g., ADC)
#include "irAVR.h" // Include the IR remote librar


// ==========================================================
//                GLOBAL VARIABLES & CONSTANTS
// ==========================================================



// ==========================================================
//                UTILITY FUNCTIONS
// ==========================================================


}

// ==========================================================
//                ADC FUNCTIONS
// ==========================================================


// ==========================================================
//                TASK STRUCTURES & CONSTANTS
// ==========================================================



// ==========================================================
//                TASK STATE ENUMS
// ==========================================================



// ==========================================================
//                TASK: JOYSTICK
// ==========================================================



// ==========================================================
//                TASK: LED
// ==========================================================



// ==========================================================
//                TASK: STEPPER MOTOR
// ==========================================================




// ==========================================================
//                TIMER ISR
// ==========================================================



// ==========================================================
//                MAIN FUNCTION
// ==========================================================

int main(void) {
    // --------------------
    // PORT INITIALIZATION
    // --------------------
   

    // --------------------
    // ADC INITIALIZATION
    // --------------------
    ADC_Init();

    // --------------------
    // TASK INITIALIZATION
    // --------------------
   

    // --------------------
    // TIMER START
    // --------------------
    TimerSet(GCD_PERIOD);
    TimerOn();

    while (1) {}
    return 0;
}
