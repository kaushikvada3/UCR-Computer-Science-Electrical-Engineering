#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "timerISR.h"

unsigned char SetBit(unsigned char x, unsigned char k, unsigned char b) {
   return (b ?  (x | (0x01 << k))  :  (x & ~(0x01 << k)) );
              //   Set bit to 1           Set bit to 0
}

unsigned char GetBit(unsigned char x, unsigned char k) {
   return ((x & (0x01 << k)) != 0);
}

//TODO: Use nums[] and outNum() from the previous lab. The wiring is slightly different, modify it so it works as intended
const unsigned char nums[10] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111  // 9
};

void outNum(unsigned char num) {
    // Display digit using nums[]
    if (num < 10) {
        unsigned char segs = nums[num];
        // Segments a-g assumed on PD6, PD7, PB0-B4
        PORTD = (PORTD & 0x3F) | ((segs & 0x03) << 6); // bits a,b → PD6, PD7
        PORTB = (PORTB & 0xE0) | ((segs & 0x7C) >> 2); // bits c,d,e,f,g → PB0–PB4
    }
}

void ADC_init() {
    //TODO: use your ADC_init from previous lab
    ADMUX = (1 << REFS0);  // AVcc reference, ADC0
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // enable + ÷128 prescaler
    ADCSRB = 0x00;
}

unsigned int ADC_read(unsigned char chnl){
  //TODO: use your ADC_read from previous lab
  ADMUX = (ADMUX & 0xF0) | (chnl & 0x0F);  // Select channel
  ADCSRA |= (1 << ADSC); // Start conversion
  while (ADCSRA & (1 << ADSC)); // Wait
  return ADC; // Read result
}

// Provided map()
long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

enum states {read_input} state; //TODO: finish the enum for the SM

void Tick() {
  //TODO: declare your static variables here or declare it globally
  static unsigned char potVal;
  unsigned int adcVal = ADC_read(0);
  potVal = (unsigned char)map(adcVal, 0, 1023, 0, 9);

  // State Transistions
  //TODO: complete transitions 
  switch (state) {
    case read_input:
      state = read_input;
      break;
  }

  // State Actions
  //TODO: complete transitions
  switch (state) {
    case read_input:
      // Turn on D4, turn off D1–D3
      PORTB = (PORTB & 0xF0) | 0x01;  // PB0 = 0 (D4 ON), others = 1
      outNum(potVal); // Show number

      // RGB LED on PD0 (Red), PD1 (Green), PD2 (Blue)
      if (potVal <= 3) {
        PORTD = SetBit(PORTD, 0, 0); // Red off
        PORTD = SetBit(PORTD, 1, 0); // Green off
        PORTD = SetBit(PORTD, 2, 1); // Blue on
      }
      else if (potVal < 8) {
        PORTD = SetBit(PORTD, 0, 1); // Red on
        PORTD = SetBit(PORTD, 1, 1); // Green on
        PORTD = SetBit(PORTD, 2, 0); // Blue off
      }
      else {
        PORTD = SetBit(PORTD, 0, 1); // Red on
        PORTD = SetBit(PORTD, 1, 0); // Green off
        PORTD = SetBit(PORTD, 2, 0); // Blue off
      }
      break;
  }
}

int main(void)
{
  //TODO: initialize all outputs and inputs
    DDRB     = 0x1F;  // PB0–PB4 as output (segments + digit select)
    PORTB    = 0xFF;  // Default high

    DDRC    = 0x00;   // PC0 as ADC input (unused here, precaution)
    PORTC   = 0x00;

    DDRD   = 0xFF;    // PD0–PD7 as output (segments + RGB)
    PORTD  = 0x00;

    ADC_init();

  TimerSet(500); //TODO: Set your timer
  TimerOn();

    state = read_input;

    while (1)
    {
        Tick();      // Execute one synchSM tick
        while (!TimerFlag){}  // Wait for SM period
        TimerFlag = 0;        // Lower flag
    }

    return 0;
}