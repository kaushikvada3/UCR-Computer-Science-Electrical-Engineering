#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "timerISR.h"

#include "serialATmega.h"

void ADC_init() {
    // Use AVcc (5V) as reference; right-adjust result
    ADMUX = (1 << REFS0);

    // Enable ADC; prescaler ÷128 → 16 MHz / 128 = 125 kHz
    ADCSRA = (1 << ADEN)                    // ADC enable
           | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);  // ADPS2:0 = 111

    // Free-running mode selected; ACME = 1 per lab spec
    ADCSRB = (1 << ACME);   // ADTS2:0 default is 000 (free-running)
}

unsigned int ADC_read(unsigned char chnl) {
    // Select channel without modifying other bits
    ADMUX = (ADMUX & 0xF0) | (chnl & 0x0F);

    // Start conversion
    ADCSRA |= (1 << ADSC);

    // Wait for conversion to complete (ADSC becomes 0)
    while ((ADCSRA >> 6) & 0x01) {}

    // Read result: must read ADCL first, then ADCH
    uint8_t low = ADCL;
    uint8_t high = ADCH;

    return (high << 8) | low;
}

void Tick() {
    unsigned int raw = ADC_read(0);  // Read from A0 (PC0)
    serial_print_uint(raw);
    serial_print_char('\n');
}

int main(void)
{
    // Set all ports as input with no pull-ups (safe defaults)
    DDRC  = 0x00;   PORTC = 0x00;   // Analog inputs
    DDRB  = 0x00;   PORTB = 0x00;
    DDRD  = 0x00;   PORTD = 0x00;

    ADC_init();
    TimerSet(500);   // Set timer to 500ms
    TimerOn();

    serial_init(9600);  // Start serial output

    while (1)
    {
        Tick();                   // Execute one SynchSM tick
        while (!TimerFlag) {}    // Wait for SM period
        TimerFlag = 0;
    }

    return 0;
}