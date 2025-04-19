/*        Kaushik Vada & vvada002@ucr.edu:

*          Discussion Section: 021

*         Assignment: Lab #2  Exercise #3

*         Exercise Description: [count in binary from 0–15 using 2 buttons and 4 LEDs and use the 7-segment display to display in hexadecimal]

*         I acknowledge all content contained herein, excluding template or example code, is my own original work.

*         Demo Link:

*/

#include <avr/io.h>
#include <util/delay.h>

enum states { start, increment, decrement } state;

int num = 0;
unsigned char prev = 0;

// 7-segment wiring:
// A = PB0, B = PB1, C = PB2, D = PB3, E = PB4
// F = PD6, G = PD7
int digitMasks[] = {
    0b00011111, 0b01000000, // 0
    0b00000110, 0b00000000, // 1
    0b00011011, 0b10000000, // 2
    0b00001111, 0b10000000, // 3
    0b00000110, 0b11000000, // 4
    0b00001101, 0b11000000, // 5
    0b00011101, 0b11000000, // 6
    0b00000111, 0b00000000, // 7
    0b00011111, 0b11000000, // 8
    0b00001111, 0b11000000, // 9
    0b00010111, 0b11000000, // A
    0b00011100, 0b11000000, // b
    0b00011001, 0b01000000, // C
    0b00011110, 0b10000000, // d
    0b00011001, 0b11000000, // E
    0b00010001, 0b11000000  // F
};

void changeDigit(int n) {
    n *= 2;

    // Clear segment bits before updating
    PORTB &= 0b11100000;  // Clear PB0–PB4 (A–E)
    PORTD &= 0b00111111;  // Clear PD6–PD7 (F, G)

    // Set segments
    PORTB |= digitMasks[n];      // A–E
    PORTD |= digitMasks[n + 1];  // F, G
}

void Tick() {
    unsigned char right = PINC & 0x02;  // PC1 (A1)
    unsigned char left  = PINC & 0x01;  // PC0 (A0)

    // State transitions
    switch(state) {
        case start:
            if (right && !prev) state = decrement;
            else if (left && !prev) state = increment;
            break;
        case increment:
        case decrement:
            state = start;
            break;
        default:
            state = start;
            break;
    }

    // State actions
    switch(state) {
        case start:
            break;

        case increment:
            if (num < 15) {
                PORTD += 4;  // Move binary LEDs
                num++;
            } else {
                PORTD = 0b00000011;
                num = 0;
            }
            changeDigit(num);
            break;

        case decrement:
            if (num > 0) {
                PORTD -= 4;
                num--;
            } else {
                PORTD = 0b00111111;
                num = 15;
            }
            changeDigit(num);
            break;

        default:
            break;
    }

    prev = left | right;
}

int main(void) {
    // Buttons on PC0 and PC1
    DDRC  = 0b00000000;   // Input
    PORTC = 0b11110011;   // Enable pull-ups

    // PB0–PB4 (A–E) as output
    DDRB  = 0b00011111;
    PORTB = 0;  // Init low

    // PD2–PD5 = binary LEDs, PD6–PD7 = F, G
    DDRD  = 0b11111100;
    PORTD = 0b00000011;  // PD0/PD1 left untouched

    state = start;
    changeDigit(0);  // Display 0 at startup

    while (1) {
        Tick();
        _delay_ms(50);  //delay
    }
}