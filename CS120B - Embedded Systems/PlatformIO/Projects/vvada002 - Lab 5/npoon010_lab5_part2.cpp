/*        Your Name & E-mail: Nicholas Poon npoon010@ucr.edu

*          Discussion Section: 021

 *         Assignment: Lab #5  Exercise #2

 *         Exercise Description: [optional - include for your own benefit]

 *        

 *         I acknowledge all content contained herein, excluding template or example code, is my own original work.

 *

 *         Demo Link: https://www.youtube.com/watch?v=csQXxLhSPu4
 */
#include "timerISR.h"
#include "helper.h"
#include "periph.h"
#include "serialATmega.h"
//TODO: declare variables for cross-task communication
// TODO: Change this depending on which exercise you are doing.
// Exercise 2: 5 tasks


long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int nums[17] = {0b1111110, 0b0110000, 0b1101101, 0b1111001, 0b0110011, 0b1011011, 0b1011111, 0b1110000, 0b1111111, 0b1111011, 0b1110111, 0b0011111, 0b1001110, 0b0111101, 0b1001111, 0b1000111, 0b000000000}; 
// a  b  c  d  e  f  g

void outNum(int num){
	  PORTD = nums[num] << 1;
  	PORTB = SetBit(PORTB, 1 ,nums[num]&0x01);
}

#define NUM_TASKS 5
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
int getTen(int n){
  int tens = n / 10;    // 26 / 10 = 2
  return tens;
}
int getOne(int n){
  int ones = n % 10;    // 26 % 10 = 6
  return ones;
}

int sonarVal;
int INCH;
unsigned short cnt = 0;
unsigned char isInches = 0; // 0 = cm, 1 = inches
int threshold_close = 8;
int threshold_far = 12;

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

// task 4
enum RED_States {RED_start, RED_check};
int cycle_count = 0;
int red_duty = 0;

int RED_Tick(int state) {
  switch (state) {
    case RED_start:
      state = RED_check;
      break;

    case RED_check:
      // Update duty cycle based on sonarVal
      if (sonarVal < threshold_close) {
        red_duty = 100;
      } else if (sonarVal <= threshold_far) {
        red_duty = 90;
      } else {
        red_duty = 0;
      }

      // PWM logic: assume 10ms period divided into 10 ticks
      if (cycle_count < (red_duty / 10)) {
        PORTC |= (1 << PC3); // RED ON (pin PC3)
      } else {
        PORTC &= ~(1 << PC3); // RED OFF
      }

      cycle_count++;
      if (cycle_count >= 10) {
        cycle_count = 0; // Reset after full PWM period
      }
      break;
  }
  return state;
}

// //task 5
enum GREEN_States {GREEN_start, GREEN_check};
int green_cycle_count = 0;
int green_duty = 0;

int GREEN_Tick(int state) {
  switch (state) {
    case GREEN_start:
      state = GREEN_check;
      break;

    case GREEN_check:
      // Determine duty cycle based on sonarVal
      if (sonarVal < threshold_close) {
        green_duty = 0;
      } else if (sonarVal <= threshold_far) {
        green_duty = 30;
      } else {
        green_duty = 100;
      }

      // PWM logic (assuming 10ms cycle, 1ms per tick)
      if (green_cycle_count < (green_duty / 10)) {
        PORTC |= (1 << PC4); // GREEN ON
      } else {
        PORTC &= ~(1 << PC4); // GREEN OFF
      }

      green_cycle_count++;
      if (green_cycle_count >= 10) {
        green_cycle_count = 0;
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
  DDRC = 0b00000100;
  PORTC = 0b00000011;
  DDRD = 0xFF;
  PORTD = 0x00;
  ADC_init(); // initializes ADC
  sonar_init(); // initializes sonar
  serial_init(9600);
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

  tasks[3].period = 1;
  tasks[3].state = RED_start;
  tasks[3].elapsedTime = tasks[3].period;
  tasks[3].TickFct = &RED_Tick;

  tasks[4].period = 1;
  tasks[4].state = GREEN_start;
  tasks[4].elapsedTime = tasks[4].period;
  tasks[4].TickFct = &GREEN_Tick;
  TimerSet(GCD_PERIOD);
  TimerOn();
  while (1) {}
  return 0;
}