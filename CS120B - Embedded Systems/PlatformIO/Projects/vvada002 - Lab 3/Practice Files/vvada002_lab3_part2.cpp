/*
    Kaushik Vada & vvada002@ucr.edu
    Discussion Section: 021
    Assignment: Lab 3 Exercise 2
    Exercise: potentiometer → map → 1‑digit 7‑segment in AM/FM modes
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "timerISR.h"

//— helper to read/write single bits —
unsigned char SetBit(unsigned char x, unsigned char k, unsigned char b) {
    return b ? (x | (1 << k)) : (x & ~(1 << k));
}
unsigned char GetBit(unsigned char x, unsigned char k) {
    return (x & (1 << k)) != 0;
}

//— segment patterns for 0–F —
// PB0=A, PB1=B, PB2=C, PB3=D, PB4=E ; PD6=F, PD7=G
const unsigned char digitMasks[16][2] = {
    {0b00011111, 0b01000000},  // 0: A,B,C,D,E on; F=0,G on
    {0b00000110, 0b00000000},  // 1
    {0b00011011, 0b10000000},  // 2
    {0b00001111, 0b10000000},  // 3
    {0b00000110, 0b11000000},  // 4: A off, B,C on, D,E off, F,G on
    {0b00001101, 0b11000000},  // 5
    {0b00011101, 0b11000000},  // 6
    {0b00000111, 0b00000000},  // 7
    {0b00011111, 0b11000000},  // 8
    {0b00001111, 0b11000000},  // 9
    {0b00010111, 0b11000000},  // A
    {0b00011100, 0b11000000},  // b
    {0b00011001, 0b01000000},  // C
    {0b00011110, 0b10000000},  // d
    {0b00011001, 0b11000000},  // E
    {0b00010001, 0b11000000}   // F
};

// write one hex digit to the display
void changeDigit(unsigned char val) {
    // clear PB0–PB4, PD6–PD7
    PORTB &= ~0x1F;
    PORTD &= ~((1<<PD6)|(1<<PD7));
    // set segments
    PORTB |= digitMasks[val][0];
    PORTD |= digitMasks[val][1];
}

//—— 1) Initialize ADC ——
void ADC_init() {
    ADMUX  = (1 << REFS0);                               // AVcc ref, right‑adjust
    ADCSRA = (1 << ADEN)                                 // enable ADC
           | (1 << ADPS2)|(1 << ADPS1)|(1 << ADPS0);     // prescaler = 128
    ADCSRB = (1 << ACME);                                // ACME=1, free‑running off
}

//—— 2) Read potentiometer on A0 ——
unsigned int ADC_read(unsigned char chnl) {
    ADMUX  = (ADMUX & 0xF0) | (chnl & 0x0F);   // select channel
    ADCSRA |= (1 << ADSC);                     // start conversion
    while (ADCSRA & (1 << ADSC)) { }           // wait for completion
    uint8_t low  = ADCL;
    uint8_t high = ADCH;
    return (high << 8) | low;                 // combine to 10‑bit value
}

//—— provided map() ——
long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min)*(out_max - out_min)/(in_max - in_min) + out_min;
}

//—— 4) State machine for AM/FM ——
enum states { INIT, AM, FM } state;

void Tick() {
    static unsigned char prevBtn = 0;
    unsigned char btn = GetBit(PINC, PC1);        // digital button on PC1
    unsigned int raw = ADC_read(0);               // analog pot on PC0/A0

    // transitions
    switch (state) {
        case INIT: state = AM; break;
        case AM:  if (btn && !prevBtn) state = FM; break;
        case FM:  if (btn && !prevBtn) state = AM; break;
    }
    prevBtn = btn;

    // actions: map raw → hex digit
    unsigned char val;
    if (state == AM) {
        val = (unsigned char)map(raw, 0, 1023, 0, 9);
    } else {
        val = (unsigned char)map(raw, 0, 1023, 10, 15);
    }

    changeDigit(val);
}

int main(void) {
    // 5) init I/O:
    //   PB0–PB4 outputs for A–E segments
    //   PD6–PD7 outputs for F–G segments
    //   PC0 input (ADC), PC1 input w/pull‑up (button)
    DDRB  = 0x1F;     PORTB  = 0x00;
    DDRD  = (1<<PD6)|(1<<PD7); PORTD = 0x00;
    DDRC  = 0x00;     PORTC  = (1<<PC1);

    ADC_init();
    state = INIT;

    // 6) timer every 500 ms
    TimerSet(500);
    TimerOn();

    while (1) {
        // wait for next tick
        while (!TimerFlag) { }
        TimerFlag = 0;
        Tick();
    }
    return 0;
}