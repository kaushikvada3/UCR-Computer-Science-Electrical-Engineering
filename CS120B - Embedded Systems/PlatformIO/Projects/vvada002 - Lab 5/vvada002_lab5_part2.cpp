/*        Your Name & E-mail: Kaushik Vada vvada002@ucr.edu

*          Discussion Section: 021

 *         Assignment: Lab #5  Exercise #2

 *         Exercise Description: [optional - include for your own benefit]

 *        

 *         I acknowledge all content contained herein, excluding template or example code, is my own original work.

 *

 *         Demo Link: 
 */

 #include "timerISR.h"
 #include "helper.h"
 #include "periph.h"
 #include "serialATmega.h"
 
 // Function to map a value from one range to another
 long map(long x, long in_min, long in_max, long out_min, long out_max) {
   return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
 }
 
 // Array for 7-segment display encoding
 int nums[17] = {0b1111110, 0b0110000, 0b1101101, 0b1111001, 0b0110011, 0b1011011, 0b1011111, 0b1110000, 
                 0b1111111, 0b1111011, 0b1110111, 0b0011111, 0b1001110, 0b0111101, 0b1001111, 0b1000111, 
                 0b000000000};
 
 // Function to output a number to the 7-segment display
 void outNum(int num) {
   PORTD = nums[num] << 1;
   PORTB = SetBit(PORTB, 1, nums[num] & 0x01);
 }
 
 // Define the number of tasks
 #define NUM_TASKS 5
 
 // Task structure for state machines
 typedef struct _task {
   signed char state; // Current state of the task
   unsigned long period; // Task period
   unsigned long elapsedTime; // Time since the last tick
   int (*TickFct)(int); // Pointer to the task's tick function
 } task;
 
 // Declare the tasks array globally
 task tasks[NUM_TASKS]; // Array to hold all tasks
 
 // Global variables for distance thresholds
 const unsigned long GCD_PERIOD = 1;
 int threshold_close = 8; // Threshold for close distance
 int threshold_far = 12;  // Threshold for far distance
 
 // Variables for distance measurement
 int sonarVal; // Distance in cm
 int INCH;     // Distance in inches
 unsigned char isInches = 0; // 0 = cm, 1 = inches
 
 // Helper functions for digit extraction
 int getTen(int n) { return n / 10; }
 int getOne(int n) { return n % 10; }
 
 // Task 1: Ultrasonic Sensor State Machine
 enum US_States { US_start };
 int US_Tick(int state) {
   switch (state) {
     case US_start:
       sonarVal = sonar_read(); // Read distance in cm
       INCH = (int)(sonarVal * 0.3937); // Convert to inches
       break;
   }
   return state;
 }
 
 // Task 2: 7-Segment Display State Machine
 enum D_States { D_start, buffer, ten, one };
 int curr = ten;
 int D_Tick(int state) {
   switch (state) {
     case D_start:
       state = ten; // Start with the tens digit
       break;
 
     case buffer:
       if (curr == ten) {
         PORTB |= 0b00001100; // Enable the ones digit
         state = one;
         curr = one;
       } else if (curr == one) {
         PORTB |= 0b00001100; // Enable the tens digit
         state = ten;
         curr = ten;
       }
       break;
 
     case ten:
       if (isInches) {
         sonarVal = INCH; // Use inches if toggled
       }
       state = buffer;
       curr = ten;
       break;
 
     case one:
       if (isInches) {
         sonarVal = INCH; // Use inches if toggled
       }
       state = buffer;
       curr = one;
       break;
   }
 
   switch (state) {
     case D_start:
       break;
 
     case buffer:
       break;
 
     case ten:
       if (isInches) {
         outNum(getTen(INCH)); // Display the tens digit for inches
         PORTB &= ~0b00001000; // Enable tens digit
         PORTB |= 0b00000100;  // Disable ones digit
       } else {
         outNum(getTen(sonarVal)); // Display the tens digit for cm
         PORTB &= ~0b00001000; // Enable tens digit
         PORTB |= 0b00000100;  // Disable ones digit
       }
       break;
 
     case one:
       if (isInches) {
         outNum(getOne(INCH)); // Display the ones digit for inches
         PORTB &= ~0b00000100; // Enable ones digit
         PORTB |= 0b00001000;  // Disable tens digit
       } else {
         outNum(getOne(sonarVal)); // Display the ones digit for cm
         PORTB &= ~0b00000100; // Enable ones digit
         PORTB |= 0b00001000;  // Disable tens digit
       }
       break;
   }
 
   return state;
 }
 
 // Task 3: Button Toggle State Machine
 enum LB_States { LB_start, LB_wait, LB_pressed };
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
 
 // Task 4: Red LED PWM State Machine
 enum RED_States { RED_start, RED_check };
 int cycle_count = 0;
 int red_duty = 0;
 
 int RED_Tick(int state) {
   switch (state) {
     case RED_start:
       state = RED_check;
       break;
 
     case RED_check:
       // Adjust duty cycle based on distance
       if (sonarVal < threshold_close) {
         red_duty = 100; // Full brightness
       } else if (sonarVal <= threshold_far) {
         red_duty = 90; // High brightness
       } else {
         red_duty = 0; // Off
       }
 
       // PWM logic
       if (cycle_count < (red_duty / 10)) {
         PORTC |= (1 << PC3); // Turn RED ON
       } else {
         PORTC &= ~(1 << PC3); // Turn RED OFF
       }
 
       cycle_count = (cycle_count + 1) % 10; // Reset after 10ms
       break;
   }
   return state;
 }
 
 // Task 5: Green LED PWM State Machine
 enum GREEN_States { GREEN_start, GREEN_check };
 int green_cycle_count = 0;
 int green_duty = 0;
 
 int GREEN_Tick(int state) {
   switch (state) {
     case GREEN_start:
       state = GREEN_check;
       break;
 
     case GREEN_check:
       // Adjust duty cycle based on distance
       if (sonarVal < threshold_close) {
         green_duty = 0; // Off
       } else if (sonarVal <= threshold_far) {
         green_duty = 30; // Low brightness
       } else {
         green_duty = 100; // Full brightness
       }
 
       // PWM logic
       if (green_cycle_count < (green_duty / 10)) {
         PORTC |= (1 << PC4); // Turn GREEN ON
       } else {
         PORTC &= ~(1 << PC4); // Turn GREEN OFF
       }
 
       green_cycle_count = (green_cycle_count + 1) % 10; // Reset after 10ms
       break;
   }
   return state;
 }
 
 // Timer ISR to handle task scheduling
 void TimerISR() {
   for (unsigned int i = 0; i < NUM_TASKS; i++) {
     if (tasks[i].elapsedTime >= tasks[i].period) {
       tasks[i].state = tasks[i].TickFct(tasks[i].state);
       tasks[i].elapsedTime = 0;
     }
     tasks[i].elapsedTime += GCD_PERIOD;
   }
 }
 
 int main(void) {
   // Initialize I/O and peripherals
   DDRB = 0xFE; // Configure PORTB
   PORTB = 0x3D;
   DDRC = 0b00011100; // Configure PC2, PC3, PC4 as outputs
   PORTC = 0b00000011;
   DDRD = 0xFF;
   PORTD = 0x00;
   ADC_init();
   sonar_init();
   serial_init(9600);
 
   // Initialize tasks
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