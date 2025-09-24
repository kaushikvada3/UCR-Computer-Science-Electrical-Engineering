#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "timerISR.h"

#include "serialATmega.h"

void ADC_init() {
    // TODO: figure out register values
    ADMUX  = (1 << REFS0);                                            // AVcc reference, right‑adjust
    ADCSRA = (1 << ADEN)                                              // enable ADC
           | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);              // prescaler = 128
    ADCSRB = (1 << ACME);                                             // ACME = 1, free‑running off
}

unsigned int ADC_read(unsigned char chnl){
    //                              ^^^^ selects which analog channel
    ADMUX  = (ADMUX & 0xF0) | (chnl & 0x0F);                           // set MUX3:0 without touching other bits
    ADCSRA |= (1 << ADSC);                                            // start conversion
    while ((ADCSRA >> 6) & 0x01) {}                                   // wait for ADSC to clear

    uint8_t low, high;
    low  = ADCL;                                                      // read ADCL first
    high = ADCH;                                                      // then ADCH
    return ((high << 8) | low);
}

void Tick() {
    // TODO: Implement your Tick Function
    unsigned int raw = ADC_read(0);                                   // read A0 (PC0)
    serial_println(raw);                                              // print raw ADC value + newline
}

int main(void)
{
    // TODO: Initialize your I/O pins
    DDRC  = 0x00;  PORTC  = 0x00;
    DDRB  = 0x00;  PORTB  = 0x00;
    DDRD  = 0x00;  PORTD  = 0x00;

    ADC_init();
    TimerSet(500);                                                    // 500 ms period
    TimerOn();                                                        // start timerISR
    serial_init(9600);                                                // UART @ 9600 baud
    
    while (1)
    {
        Tick();                                                       // Execute one SynchSM tick
        while (!TimerFlag) {}                                        // Wait for SM period
        TimerFlag = 0;
    }

    return 0;
}