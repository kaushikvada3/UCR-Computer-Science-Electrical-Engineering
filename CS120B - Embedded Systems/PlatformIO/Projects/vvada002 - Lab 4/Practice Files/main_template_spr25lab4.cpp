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

//TODO: Use nums[] and outNum[] from the previous lab. The wiring is slightly different, modify it so it works as intended

void ADC_init() {
    //TODO: use your ADC_init from previous lab
}



unsigned int ADC_read(unsigned char chnl){
  //TODO: use your ADC_read from previous lab
}

// Provided map()
long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

enum states {} state; //TODO: finish the enum for the SM

void Tick() {
  //TODO: declare your static variables here or declare it globally
  
  // State Transistions
  //TODO: complete transitions 
  switch (state) {
    
  }

  // State Actions
  //TODO: complete transitions
  switch (state) {
    
  }
}


int main(void)
{
  //TODO: initialize all outputs and inputs
    DDRB     = ;
    PORTB    = ;
  
    DDRC    = ;
    PORTC   = ;

    DDRD   = ;
    PORTD  = ;

    ADC_init();

  TimerSet(); //TODO: Set your timer
  TimerOn();
    while (1)
    {
      
	      Tick();      // Execute one synchSM tick
        while (!TimerFlag){}  // Wait for SM period
        TimerFlag = 0;        // Lower flag
    }

    return 0;
}