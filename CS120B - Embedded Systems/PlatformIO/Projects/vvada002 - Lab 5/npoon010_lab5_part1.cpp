/*        Your Name & E-mail: Nicholas Poon npoon010@ucr.edu

*          Discussion Section: 021

 *         Assignment: Lab #5  Exercise #1

 *         Exercise Description: [optional - include for your own benefit]

 *        

 *         I acknowledge all content contained herein, excluding template or example code, is my own original work.

 *

 *         Demo Link: https://www.youtube.com/watch?v=kPgghtCA__I 
 */
#include "timerISR.h"
#include "helper.h"
#include "periph.h"
//TODO: declare variables for cross-task communication
// TODO: Change this depending on which exercise you are doing.
// Exercise 1: 3 tasks
// Exercise 2: 5 tasks
// Exercise 3: 7 tasks

long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int nums[17] = {0b1111110, 0b0110000, 0b1101101, 0b1111001, 0b0110011, 0b1011011, 0b1011111, 0b1110000, 0b1111111, 0b1111011, 0b1110111, 0b0011111, 0b1001110, 0b0111101, 0b1001111, 0b1000111, 0b000000000}; 
// a  b  c  d  e  f  g

void outNum(int num){
	  PORTD = nums[num] << 1;
  	PORTB = SetBit(PORTB, 1 ,nums[num]&0x01);
}

#define NUM_TASKS 3
//Task struct for concurrent synchSMs implmentations
typedef struct _task{
  signed char state; //Task's current state
  unsigned long period; //Task period
  unsigned long elapsedTime; //Time elapsed since last task tick
  int (*TickFct)(int); //Task tick function
} task;

//TODO: Define Periods for each task
// e.g. const unsined long TASK1_PERIOD = <PERIOD>
const unsigned long GCD_PERIOD = 1;
int sonarVal;
int INCH;
int getTen(int n){
  int tens = n / 10;    // 26 / 10 = 2
  return tens;
}
int getOne(int n){
  int ones = n % 10;    // 26 % 10 = 6
  return ones;
}


unsigned short cnt = 0;
unsigned char isInches = 0; // 0 = cm, 1 = inches
task tasks[NUM_TASKS]; // declared task array with NUM_TASKS amount of tasks
// task 1
enum US_States{US_start};
int US_Tick(int state){
  switch(state){
    case US_start:
      sonarVal = sonar_read();
      INCH = (int)(sonarVal * 0.3937); // convert cm to inches

  }
  switch(state){
    case US_start:
      break;
  }
  return state; 
}
// task 2
enum D_States {D_start, buffer, ten, one};
int curr = ten;
int D_Tick(int state) {
  switch(state){
    case D_start:
      state = ten;
      break;
    case buffer:
      if(curr == ten){
        PORTB = PORTB | 0b00001100;
        state = one;
        curr = one;
      }
      else if(curr == one){
        PORTB = PORTB | 0b00001100;
        state = ten;
        curr = ten;
      }
      break;
    case ten:
      if (isInches) {
        sonarVal = INCH; // convert cm to inches
      }
      state = buffer;
      curr = ten;
      break;
    case one:
      if (isInches) {
        sonarVal = INCH; // convert cm to inches
      }
      state = buffer;
      curr = one;
      break;
  }
  switch(state){
    case D_start:
      break;
    case buffer:
      break;
    case ten:
    if(isInches){
      outNum(getTen(INCH));
      PORTB = PORTB & 0b11111011;
      if(INCH >= 10){
        outNum(getTen(INCH));
      }
      else{
        outNum(17);
      }
      PORTB = PORTB & 0b11110111;
    }
    else{
      outNum(getTen(sonarVal));
      PORTB = PORTB & 0b11111011;
      if(sonarVal >= 10){
        outNum(getTen(sonarVal));
      }
      else{
        outNum(17);
      }
      PORTB = PORTB & 0b11110111;
    }
      
      break;
    case one:
      if(isInches){
        outNum(getOne(INCH));
        PORTB = PORTB & 0b11111011;
      }
      else{
        outNum(getOne(sonarVal));
        PORTB = PORTB & 0b11111011;
      }
      break;    
  }
  return state;
}
// task 3
enum LB_States {LB_start, LB_wait, LB_pressed};
int LB_Tick(int state) {
  switch (state) {
    case LB_start:
      state = LB_wait;
      break;
    case LB_wait:
      if (PINC & 0x01) {
        isInches ^= 1; // toggle between 0 and 1
        state = LB_pressed;
      }
      break;
    case LB_pressed:
      if (!(PINC & 0x01)) {
        state = LB_wait;
      }
      break;
  }
  return state;
}

void TimerISR() {
//TODO: sample inputs here
  for ( unsigned int i = 0; i < NUM_TASKS; i++ ) { // Iterate through each task in the task array
  if ( tasks[i].elapsedTime == tasks[i].period ) { // Check if the task is ready to tick
    tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
    tasks[i].elapsedTime = 0; // Reset the elapsed time for the next tick
  }
  tasks[i].elapsedTime += GCD_PERIOD; // Increment the elapsed time by GCD_PERIOD
  }
}
int result = 0;



int main(void) {
  //TODO: initialize all your inputs and ouputs
  DDRB = 0xFE; // 0b11111110
  PORTB = 0x3D; // 0b00111101
  DDRC = 0b00111100;
  PORTC = 0b11000011;
  DDRD = 0xFF;
  PORTD = 0x00;
  ADC_init(); // initializes ADC
  sonar_init(); // initializes sonar
  //TODO: Initialize tasks here
  // e.g. tasks[0].period = TASK1_PERIOD
  // tasks[0].state = ...
  // tasks[0].elapsedTime = ...
  // tasks[0].TickFct = &task1_tick_function;
  tasks[0].period = 1000;
  tasks[0].state = US_start;
  tasks[0].elapsedTime = tasks[0].period;
  tasks[0].TickFct = &US_Tick;
  
  tasks[1].period = 1;
  tasks[1].state = D_start;
  tasks[1].elapsedTime = tasks[1].period;
  tasks[1].TickFct = &D_Tick;

  tasks[2].period = 200;
  tasks[2].state = LB_start;
  tasks[2].elapsedTime = tasks[2].period;
  tasks[2].TickFct = &LB_Tick;
  TimerSet(GCD_PERIOD);
  TimerOn();
  while (1) {}
  return 0;
}