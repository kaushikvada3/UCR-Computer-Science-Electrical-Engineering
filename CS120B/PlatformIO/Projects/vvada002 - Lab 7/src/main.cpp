#include "timerISR.h"
#include "helper.h"
#include "periph.h"
#include "serialATmega.h"

int phases[8] = {0b0001, 0b0011, 0b0010, 0b0110, 0b0100, 0b1100, 0b1000, 0b1001}; //8 phases of the stepper motor step
#define NUM_TASKS 5//TODO: Change to the number of tasks being used
//Task struct for concurrent synchSMs implmentations
typedef struct _task{
  signed char state; //Task's current state
  unsigned long period; //Task period
  unsigned long elapsedTime; //Time elapsed since last task tick
  int (*TickFct)(int); //Task tick function
} task;
//TODO: Define Periods for each task
// e.g. const unsined long TASK1_PERIOD = <PERIOD>

long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

const unsigned long LED_Period = 10;
const unsigned long BUZZ_Period = 50;
unsigned long STEP_Period = 2;
unsigned long JS_Period = 1;
unsigned long SERVO_Period = 2;
unsigned short ServoPot = 0;
const unsigned long GCD_PERIOD = 1;//TODO:Set the GCD Period
task tasks[NUM_TASKS]; // declared task array with 5 tasks

unsigned char i = 0;
unsigned char j = 0;
int k = 0;
unsigned char p = 0;
unsigned char back_speed = 0;
unsigned char speed = 0;
long period = 0;
unsigned char direction = 0;
enum LED_States{LED_start, LED_off, LED_on};
int LED_Tick(int state){
  switch(state){
    case LED_start:
      state = LED_off;
      break;
    case LED_off:
      if((PINC >> 3 & 0x01) |(PINC >> 4 & 0x01)){
        state = LED_on;
      }
      break;
    case LED_on:
      if(!((PINC >> 3 & 0x01) |(PINC >> 4 & 0x01))){
        state = LED_off;
      }
      break;
  }
  switch(state){
    case LED_start:
      break;
    case LED_off:
      PORTD &= 0b01000000;
      PORTB &= 0b11111110;
      j = 0;
      i = 0;
      break;
    case LED_on:
      if((PINC >> 3 & 0x01) && (PINC >> 4 & 0x01)){
        PORTD &= 0b01000000;
        PORTB &= 0b11111110;
        j = 0;
        i = 0;
      }
      else if(PINC >> 4 & 0x01){
        if(i<40){
          if(j == 0){
            PORTD |= 0x10;
          }
          if(j == 1){
            PORTD |= 0x18;
          }
          if(j == 2){
            PORTD |= 0x1C;
          }
          i++;
        }
        else{
          PORTD &= 0;
          i = 0;
          if(j == 3){
            j = 0;
          }
          else{
            j++;
          }
        }
      }
      else if(PINC >> 3 & 0x01){
        if(i<40){
          if(j == 0){
            PORTB |= 0x01;
          }
          if(j == 1){
            PORTB |= 0x01;
            PORTD |= 0x80;
          }
          if(j == 2){
            PORTB |= 0x01;
            PORTD |= 0xA0;
          }
          i++;
        }
        else{
          PORTD &= 0;
          PORTB &= 0;
          i = 0;
          if(j == 3){
            j = 0;
          }
          else{
            j++;
          }
        }
      }
      break;
  }
  return state;
}
enum BUZZ_States{BUZZ_start, BUZZ_wait, BUZZ_back};
int BUZZ_Tick(int state){
  switch(state){
    case BUZZ_start:
      state = BUZZ_wait;
      break;
    case BUZZ_wait:
      p = 0;
      if(direction < 17){
        state = BUZZ_back;
      }
      else if(!(PINC >> 2 & 0x01)){
        TCCR0B = (TCCR0B & 0xF8) | 0x04; //set prescaler to 8
      }
      else{
        TCCR0B = (TCCR0B & 0xF8) | 0x00; //set prescaler to 8
      }
      break;
    case BUZZ_back:
      if(direction >= 17){
        state = BUZZ_wait;
      }
      break;
  }
  switch(state){
    case BUZZ_start:
      break;
    case BUZZ_wait:
      break;
    case BUZZ_back:
      if(p < 30){
        TCCR0B = (TCCR0B & 0xF8) | 0x00; //set prescaler to 8
        p++;
      }
      else if(p >= 30){
        if(p < 50){
          TCCR0B = (TCCR0B & 0xF8) | 0x03; //set prescaler to 8
          p++;
        }
        else{
          p = 0;
        }
      }
      break;
  }
  return state;
}

enum STEP_States {STEP_start, STEP_stand, STEP_turn_cw, STEP_turn_ccw};
int STEP_Tick(int state){
  switch(state){ 
    case STEP_start:
      state = STEP_stand;
      break;
    case STEP_stand:
        k = 0;
        PORTB = (PORTB & 0x03);
        if(direction > 17){
          state = STEP_turn_cw;
          period = speed;
        }
        else if(direction < 17){
          state = STEP_turn_ccw;
          period = speed;
        }
      break;
    case STEP_turn_cw:
      if(direction == 17){
        state = STEP_stand;
      }
      break;
    case STEP_turn_ccw:
      if(direction == 17){
        state = STEP_stand;
      }
      break;

  }
  switch(state){
    case STEP_start:
      break;
    case STEP_stand:
     break;
    case STEP_turn_cw:
        tasks[2].period = speed;
        PORTB = (PORTB & 0x03) | phases[k] << 2;
        k++;
        if(k>7){ 
          k = 0;
        } 
        break;
    case STEP_turn_ccw:
        tasks[2].period = back_speed;
        PORTB = (PORTB & 0x03) | phases[k] << 2;
        k--;
        if(k<0){ 
          k = 7;
        }
      break;
  }
  return state;
}
enum JS_States{JS_start, JS_read};
int JS_Tick(int state){
  switch(state){
    case JS_start:
      state = JS_read;
      break;
    case JS_read:
      break;
  }
  switch(state){
    case JS_start:
      state = JS_read;
      break;
    case JS_read:
      speed = map(map(ADC_read(0),10,1023,2,30),17,30,31,0) - 1;
      back_speed = map(speed,32,65,30,2);
      direction = map(ADC_read(0),10,1023,2,30);
      ServoPot = map(ADC_read(1),10,1023,5300,1500);
      break;
  }
  return state;
}

enum SERVO_States{SERVO_start, SERVO_run};
int SERVO_Tick(int state){
  switch(state){
    case SERVO_start:
      state = SERVO_run;
      break;
    case SERVO_run:
      break;
  }
  switch(state){
    case SERVO_start:
      break;
    case SERVO_run:
      OCR1A = ServoPot;
      break;
  }
  return state;
}
void TimerISR() {
for (unsigned int i = 0; i < NUM_TASKS; i++) { // Iterate through each task in the task array
  if (tasks[i].elapsedTime == tasks[i].period) { // Check if the task is ready to tick
    tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
    tasks[i].elapsedTime = 0; // Reset the elapsed time for the next tick
  }
  tasks[i].elapsedTime += GCD_PERIOD; // Increment the elapsed time by GCD_PERIOD
}
}
int stages[8] = {0b0001, 0b0011, 0b0010, 0b0110, 0b0100, 0b1100, 0b1000,0b1001};//Stepper motor phases
//TODO: Create your tick functions for each task
int main(void) {
  //TODO: initialize all your inputs and ouputs
  DDRB = 0xFF;
  PORTB = 0x00;
  DDRC = 0x00;  
  PORTC = 0xFF;
  DDRD = 0xFF;
  PORTD = 0x00;
  ADC_init(); // initializes ADC
  //TODO: Initialize the buzzer timer/pwm(timer0)
  OCR0A = 64; //sets duty cycle to 50% since TOP is always 256
  TCCR0A |= (1 << COM0A1);// use Channel A
  TCCR0A |= (1 << WGM01) | (1 << WGM00);// set fast PWM Mode
  //TODO: Initialize the servo timer/pwm(timer1)
  TCCR1A |= (1 << WGM11) | (1 << COM1A1); //COM1A1 sets it to channel A
  TCCR1B |= (1 << WGM12) | (1 << WGM13) | (1 << CS11); //CS11 sets the prescaler to be 8
  //WGM11, WGM12, WGM13 set timer to fast pwm mode

  ICR1 = 39999; //20ms pwm period
  //TODO: Initialize tasks here
  // e.g.
  tasks[0].period = LED_Period;
  tasks[0].state = LED_start;
  tasks[0].elapsedTime =  tasks[0].period;
  tasks[0].TickFct = &LED_Tick;

  tasks[1].period = BUZZ_Period;
  tasks[1].state = BUZZ_start;
  tasks[1].elapsedTime = tasks[1].period;
  tasks[1].TickFct = &BUZZ_Tick;

  tasks[2].period = STEP_Period;
  tasks[2].state = STEP_start;
  tasks[2].elapsedTime = tasks[2].period;
  tasks[2].TickFct = &STEP_Tick;

  tasks[3].period = JS_Period;
  tasks[3].state = JS_start;
  tasks[3].elapsedTime = tasks[3].period;
  tasks[3].TickFct = &JS_Tick;

  tasks[4].period = SERVO_Period;
  tasks[4].state = SERVO_start;
  tasks[4].elapsedTime = tasks[4].period;
  tasks[4].TickFct = &SERVO_Tick;

  TimerSet(GCD_PERIOD);
  TimerOn();
  serial_init(9600);
  while (1) {}//serial_println(direction);
  return 0;
}