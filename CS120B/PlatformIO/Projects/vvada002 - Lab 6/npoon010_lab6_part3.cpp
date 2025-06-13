/*        Your Name & E-mail: Nicholas Poon npoon010@ucr.edu

*          Discussion Section: 021

 *         Assignment: Lab #6  Exercise #3

 *         Exercise Description: [optional - include for your own benefit]

 *        

 *         I acknowledge all content contained herein, excluding template or example code, is my own original work.

 *

 *         Demo Link: https://www.youtube.com/watch?v=lDqzG_Nh_WA  
 */
#include <avr/interrupt.h>
#include <avr/io.h>
#include "timerISR.h"

int phases[8] = {0b0001, 0b0011, 0b0010, 0b0110, 0b0100, 0b1100, 0b1000, 0b1001}; //8 phases of the stepper motor step
//TODO: declare variables for cross-task communication here
unsigned short VRx = 0;
unsigned short VRy = 0;
unsigned char count = 0;
bool turn = false;
bool wrong = false;
bool direction = false;
unsigned char c = 0;
unsigned char p = 0;
unsigned char j = 0;
unsigned short step_p = 0;
char code[4];
char ans[4] = {'U','D','L','R'};
unsigned char flag = 0;
unsigned cracked = 0;

void empty(char* arr, unsigned char size) {
  if (arr[0] == '\0') return;
  for (unsigned char i = 0; i < size; ++i) {
      arr[i] = '\0';
  }
}

void change(char* arr, unsigned char size){
  if (arr[0] == '\0') return;
  for (unsigned char i = 0; i < size; ++i) {
      ans[i] = arr[i];
  }
}

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
#define NUM_TASKS 3 /*number of task here*/
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
const unsigned long STEP_PERIOD = 1;
//const unsigned long /*etc*/
const unsigned long GCD_PERIOD = 1;/* Calulate GCD of tasks */
task tasks[NUM_TASKS]; // declared task array with 5 tasks (2 in exercise 1)
//TODO: Define, for each task:
// (1) enums and
enum JS_States{JS_Start, JS_read, JS_unlock, JS_wrong, JS_reset};
int JS_Tick(int state){
  switch(state){
    case JS_Start:
      state = JS_read;
      break;
    case JS_read:
      if(wrong){
        state = JS_wrong;
      }
      if(!(PINC >> 4 & 0x01) && direction){
        empty(code,4);
        j = 0;
        state = JS_reset;
      }
      break;
    case JS_unlock:
      if(!turn){
        state = JS_read;
      }
      break;
    case JS_wrong:
      if(!wrong){
        state = JS_read;
      }
      break;
    case JS_reset:
      PORTB = SetBit(PORTB,1,1);
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
        flag = 1;
        code[count] = 'L';
      }
      else if(VRx > 600){ // right
        outNum(0b0000101);
        flag = 1;
        code[count] = 'R';
      }
      else if(VRy > 20){ // up
        outNum(0b0111110);
        flag = 1;
        code[count] = 'U';
      }
      else if(VRy < 1){ // down
        outNum(0b0111101);
        flag = 1;
        code[count] = 'D';
      }
      else{
        outNum(0b0000001);
        if(flag > 0){
          if(count >= 3){
            count = 0;
            for(unsigned char i = 0; i < 4; i++){
              if(code[i] == ans[i]){
                cracked++;
              }
            }
            if(cracked == 4){
              turn = true;
              cracked = 0;
              state = JS_unlock;
            }
            else{
              cracked = 0;
              wrong = true;
            }
            empty(code,4);
          }
          else{
            count++;
          }
          flag = 0;
        }
      }
      break;
    case JS_unlock:
      break;
    case JS_reset:
      VRx = ADC_read(3);
      VRy = ADC_read(2);
      if(VRx < 400){ // left
        outNum(0b0001110);
        flag = 1;
        code[j] = 'L';
      }
      else if(VRx > 600){ // right
        outNum(0b0000101);
        flag = 1;
        code[j] = 'R';
      }
      else if(VRy > 20){ // up
        outNum(0b0111110);
        flag = 1;
        code[j] = 'U';
      }
      else if(VRy < 1){ // down
        outNum(0b0111101);
        flag = 1;
        code[j] = 'D';
      }
      else{
        outNum(0b0000001);
        if(flag > 0){
          if(j > 2){
            j = 0;
            change(code, 4);
            PORTB = SetBit(PORTB,1,0);
            state = JS_read;
          }
          else{
            j++;
          }
          flag = 0;
        }
      }
      break;
  }
  return state; 
}
enum LED_States {LED_start, LED_read, LED_cracked, LED_wrong, LED_reset};
int LED_Tick(int state){
  switch(state){
    case LED_start:
      state = LED_read;
      break;
    case LED_read:
      if(turn){
        state = LED_cracked;
      }
      if(wrong){
        state = LED_wrong;
      }
      break;
    case LED_cracked:
      if(!turn){
        PORTC &= 0xFC;
        state = LED_read;
      }
      break;
    case LED_wrong:
      if(c >= 40){
        c = 0;
        p = 0;
        wrong = false;
        state = LED_read;
      }
      p++;
      c++;
      break;
  }
  switch(state){
    case LED_start:
      break;
    case LED_read:
      PORTC &= 0xFC;
      PORTC |= count;
      break;
    case LED_cracked:
      PORTC |= 3;
      break;
    case LED_wrong:
      if(p < 5){
        PORTC |= 0x03;
      }
      else if(p >= 5){
        PORTC &= 0xFC;
        p = 0;
      }
      break;
    case LED_reset:
      
      break;
  }
  return state;
}
enum STEP_States {STEP_start, STEP_stand, STEP_turn_cw, STEP_turn_ccw};
int STEP_Tick(int state){
  switch(state){ // when cracked == 4 go to unlock mode and turn the motor once 90 degrees
    case STEP_start:
      state = STEP_stand;
      break;
    case STEP_stand:
      if(turn){
        PORTB = (PORTB & 0x03) | phases[j] << 2;
        state = STEP_turn_cw;
      }
      break;
    case STEP_turn_cw:
      if(direction){
        state = STEP_turn_ccw;
      }
      break;
    case STEP_turn_ccw:
      if(!direction){
        state = STEP_turn_ccw;
      }
      break;
    
  }
  switch(state){
    case STEP_start:
      break;
    case STEP_stand:
      if(turn){
        state = STEP_turn_cw;
      }
      break;
    case STEP_turn_cw:
      if(step_p < 900){
        if(j < 8){
          PORTB = (PORTB & 0x03) | phases[j] << 2;
        }
        else{j = 0;}
        j++;
      } 
      else{ 
        step_p = 0;
        j = 7;
        turn = false;
        direction = !direction;
        state = STEP_stand;
      }
      step_p++;
      break;
    case STEP_turn_ccw:
      if(step_p < 900){
        j--;
        if(j > 0){
          PORTB = (PORTB & 0x03) | phases[j] << 2;
        }
        else if(j == 0){
          PORTB = (PORTB & 0x03) | phases[j] << 2;
          j = 7;}
      }
      
      else{ 
        step_p = 0;
        j = 0;
        turn = false;
        direction = !direction;
        state = STEP_stand;
      }
      step_p++;
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

  tasks[2].period = STEP_PERIOD;
  tasks[2].state = STEP_start;
  tasks[2].elapsedTime = 0;
  tasks[2].TickFct = &STEP_Tick;

  TimerSet(GCD_PERIOD);
  TimerOn();
  while (1) {}
  return 0;
}
