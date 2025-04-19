/*
    Exercise 3 (updated buzzer pin)
    Active buzzer positive → Arduino digital pin 2 (PD2)
    Buzz when display = '5' in AM or 'A' in FM
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "timerISR.h"

unsigned char SetBit(unsigned char x, unsigned char k, unsigned char b) {
    return b ? (x | (1 << k)) : (x & ~(1 << k));
}
unsigned char GetBit(unsigned char x, unsigned char k) {
    return (x & (1 << k)) != 0;
}

// Segment patterns for 0–F on PB0–PB4 and PD6–PD7
const unsigned char nums[16] = {
    0b00011111, 0b00000110, 0b00011011, 0b00001111,
    0b00000110, 0b00001101, 0b00011101, 0b00000111,
    0b00011111, 0b00001111, 0b00010111, 0b00011100,
    0b00011001, 0b00011110, 0b00011001, 0b00010001
};
const unsigned char outNum[16] = {
    0b01000000,0b00000000,0b10000000,0b10000000,
    0b11000000,0b11000000,0b11000000,0b00000000,
    0b11000000,0b11000000,0b11000000,0b11000000,
    0b01000000,0b10000000,0b11000000,0b11000000
};

void ADC_init() {
    ADMUX  = (1 << REFS0);
    ADCSRA = (1 << ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
    ADCSRB = (1 << ACME);
}

unsigned int ADC_read(unsigned char chnl) {
    ADMUX  = (ADMUX & 0xF0)|(chnl & 0x0F);
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC)) { }
    uint8_t low  = ADCL, high = ADCH;
    return (high << 8) | low;
}

long map(long x,long in_min,long in_max,long out_min,long out_max) {
    return (x - in_min)*(out_max - out_min)/(in_max - in_min)+out_min;
}

enum states { INIT, AM, FM } state;

void Tick() {
    static unsigned char prevBtn = 0;
    unsigned char btn = GetBit(PINC, PC1);
    unsigned int raw = ADC_read(0);

    // Transitions
    switch (state) {
      case INIT: state = AM; break;
      case AM:  if (btn && !prevBtn) state = FM; break;
      case FM:  if (btn && !prevBtn) state = AM; break;
    }
    prevBtn = btn;

    // Map to 0–9 or 10–15
    unsigned char val = (state==AM)
        ? (unsigned char)map(raw, 0, 1023, 0, 9)
        : (unsigned char)map(raw, 0, 1023, 10, 15);

    // Update 7‑segment
    PORTB = nums[val];                                         // PB0–PB4
    PORTD = (PORTD & ~((1<<PD6)|(1<<PD7))) | outNum[val];      // PD6–PD7

    // Buzz on '5' in AM or 'A'(10) in FM, on PD2
    unsigned char buzz = (state==AM && val==5) || (state==FM && val==10);
    PORTD = SetBit(PORTD, PD2, buzz);
}

int main(void)
{
    // PB0–PB4 = segments A–E
    DDRB  = 0x1F;  PORTB = 0x00;
    // PD2 = buzzer output; PD6–PD7 = segments F–G
    DDRD  = (1<<PD2)|(1<<PD6)|(1<<PD7);
    PORTD = 0x00;
    // PC0 = ADC input; PC1 = button input w/pull‑up
    DDRC  = 0x00;  PORTC = (1<<PC1);

    ADC_init();
    state = INIT;

    TimerSet(500);
    TimerOn();

    while (1) {
        while (!TimerFlag) {}
        TimerFlag = 0;
        Tick();
    }
    return 0;
}