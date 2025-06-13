/*        Your Name & E-mail: Nicholas Poon npoon010@ucr.edu

*          Discussion Section: 021

 *         Assignment: Lab #6  Exercise #1

 *         Exercise Description: [optional - include for your own benefit]

 *        

 *         I acknowledge all content contained herein, excluding template or example code, is my own original work.

 *

 *         Demo Link: https://www.youtube.com/watch?v=o7jKVUyn3Q4 
 */
#include <avr/interrupt.h>
#include <avr/io.h>
#include "timerISR.h"

int phases[8] = {0b0001, 0b0011, 0b0010, 0b0110, 0b0100, 0b1100, 0b1000, 0b1001}; //8 phases of the stepper motor step
//TODO: declare variables for cross-task communication here
unsigned short VRx = 0;
unsigned short VRy = 0;
unsigned char button = 0;

int nums[17] = {0b1111110, 0b0110000, 0b1101101, 0b1111001, 0b0110011, 0b1011011, 0b1011111, 0b1110000, 0b1111111, 0b1111011, 0b1110111, 0b0011111, 0b1001110, 0b0111101, 0b1001111, 0b1000111,0b0000001}; 
// a  b  c  d  e  f  g
unsigned char SetBit(unsigned char x, unsigned char k, unsigned char b) {
  return (b ?  (x | (0x01 << k))  :  (x & ~(0x01 << k)) );
             //   Set bit to 1           Set bit to 0
}
void outNum(int num){
	  PORTD = num << 1;
  	PORTB = SetBit(PORTB, 0 ,num&0x01);
}
//TODO: copy-paste the ADC functions you've implemented in previous lab here
void ADC_init() {
  ADMUX = 0b01000000; //bit 7:6 -> 0:1 (AVCC ref volt)
  ADCSRA = 0b10000111; // bit7=1 -> ADC enable, bit6->conversion, bit5->(auto trigger), bit2-0 -> prescaler
  ADCSRB = 0b01000000; // bits0-2 decide mode and the 6th bit is ACME
                       // but ADEN in ADCSRA is already 0 so ACME is negligible
}
unsigned int ADC_read(unsigned char chnl){
  ADMUX &= 0xF0;
  ADMUX = ADMUX | chnl; //select the output chnl
  ADCSRA = ADCSRA | 0b01000000;
  while((ADCSRA >> 6) & 0x01) {} // wait for ADSC to clear
  uint8_t low, high;
  low = ADCL;
  high = ADCH;
  return ((high << 8) | low) ;
}
//TODO: declare the total number of tasks here
#define NUM_TASKS 2 /*number of task here*/
// Task struct for concurrent synchSMs implmentations
typedef struct _task{
signed char state; //Task's current state
unsigned long period; //Task period
unsigned long elapsedTime; //Time elapsed since last task tick
int (*TickFct)(int); //Task tick function
} task;
//TODO: Define Periods for each task
// example: const unsined long ABC_PERIOD = 123;
const unsigned long JS_PERIOD = 200;
const unsigned long LED_PERIOD = 200;
//const unsigned long /*etc*/
const unsigned long GCD_PERIOD = 100;/* Calulate GCD of tasks */
task tasks[NUM_TASKS]; // declared task array with 5 tasks (2 in exercise 1)
//TODO: Define, for each task:
// (1) enums and
enum JS_States{JS_Start, JS_read};
int JS_Tick(int state){
  switch(state){
    case JS_Start:
      state = JS_read;
      break;
    case JS_read:
      state = JS_read;
      break;
  }
  switch(state){
    case JS_Start:
      break;
    case JS_read:
      VRx = ADC_read(3);
      VRy = ADC_read(2);
      if(VRx < 400){ // left
        outNum(0b0001110);
      }
      else if(VRx > 600){ // right
        outNum(0b0000101);
      }
      else if(VRy > 20){ // up
        outNum(0b0111110);
      }
      else if(VRy < 1){ // down
        outNum(0b0111101);
      }
      else{
        outNum(0b0000001);
      }
      break;
  }
  return state; 
}
enum LED_States {LED_start, LED_read};
int LED_Tick(int state){
  switch(state){
    case LED_start:
      state = LED_read;
      break;
    case LED_read:
      if(!(PINC >> 4 & 0x01)){
        if(button < 3){
          button++;
        }
        else{
          button = 0;
        }
      }
      break;
  }
  switch(state){
    case LED_start:
      break;
    case LED_read:
      PORTC &= 0xFC;
      PORTC |= button;
      break;
  }
  return state;
}

void TimerISR() {
for ( unsigned int i = 0; i < NUM_TASKS; i++ ) { // Iterate through each task in the task array
  if ( tasks[i].elapsedTime == tasks[i].period ) { // Check if the task is ready to tick
  tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
  tasks[i].elapsedTime = 0; // Reset the elapsed time for the next tick
  }
  tasks[i].elapsedTime += GCD_PERIOD; // Increment the elapsed time by GCD_PERIOD
}
}
int main(void) {
//TODO: initialize all your inputs and ouputs
DDRC =  0b00000111;
PORTC = 0b00011000;
DDRB = 0xFF;
PORTB = 0x00;
DDRD = 0xFF;
PORTD = 0x00;
ADC_init(); // initializes ADC

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

while (1) {}
return 0;
}