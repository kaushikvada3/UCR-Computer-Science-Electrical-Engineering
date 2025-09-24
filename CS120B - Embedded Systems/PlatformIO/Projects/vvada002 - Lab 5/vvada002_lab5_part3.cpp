/*        Your Name & E-mail: Kaushik Vada vvada002@ucr.edu

*          Discussion Section: 021

 *         Assignment: Lab #5  Exercise #3

 *         Exercise Description: [optional - include for your own benefit]

 *        

 *         I acknowledge all content contained herein, excluding template or example code, is my own original work.

 *

 *         Demo Link: 
 */

 #include "timerISR.h"
 #include "helper.h"
 #include "periph.h"
 
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
 #define NUM_TASKS 7
 
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
 int tracked_distance = 10; // Default tracked distance
 int threshold_close = 8;   // Close distance threshold
 int threshold_far = 12;    // Far distance threshold
 unsigned char update_in_progress = 0; // Flag for update mode
 unsigned char blue_blink_state = 0;   // State for blue LED blinking
 int blue_blink_counter = 0;           // Counter for blue LED blinking
 
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
 
       // Turn off LEDs if update is in progress
       if (update_in_progress) {
         PORTC &= 0b11110111; // RED OFF
         PORTC &= 0b11111011; // GREEN OFF
       }
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
 
       // Turn off LEDs if update is in progress
       if (update_in_progress) {
         PORTC &= 0b11100111; // GREEN OFF
       }
       break;
   }
   return state;
 }
 
 // Task 6: Right Button State Machine
 enum RB_States { RB_start, RB_wait, RB_pressed, RB_blink };

 int RB_Tick(int state) {
   switch (state) {
     case RB_start:
       state = RB_wait;
       break;

     case RB_wait:
       if (PINC & 0x02) { // Right button pressed (PINC bit 1)
         update_in_progress = 1; // Enter update mode
         blue_blink_counter = 0; // Reset blink counter
         state = RB_blink;
       }
       break;

     case RB_blink:
       if (blue_blink_counter >= 12) { // 250ms * 12 = 3 seconds
         // Update thresholds based on the current distance
         tracked_distance = sonarVal;
         threshold_close = tracked_distance * 4 / 5; // 4/5 of the distance
         threshold_far = tracked_distance * 6 / 5;  // 6/5 of the distance
         update_in_progress = 0; // Exit update mode
         state = RB_wait;
       }
       break;

     case RB_pressed:
       if (!(PINC & 0x02)) { // Right button released
         state = RB_wait;
       }
       break;
   }
   return state;
 }

 // Task 7: Blue LED Blinking State Machine
 enum BL_States { BL_start, BL_wait, BL_toggle };

 int BL_Tick(int state) {
   switch (state) {
     case BL_start:
       state = BL_wait;
       break;

     case BL_wait:
       if (update_in_progress) {
         blue_blink_counter = 0; // Reset blink counter
         state = BL_toggle;
       }
       break;

     case BL_toggle:
       if (!update_in_progress) {
         PORTC &= ~(1 << PC5); // Turn blue LED OFF
         state = BL_wait;
       } else {
         blue_blink_state ^= 1; // Toggle blue LED state
         if (blue_blink_state) {
           PORTC |= (1 << PC5); // Turn blue LED ON
         } else {
           PORTC &= ~(1 << PC5); // Turn blue LED OFF
         }
         blue_blink_counter++; // Increment blink counter
       }
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
   DDRC = 0b00111100; // Configure PC2, PC3, PC4 as outputs
   PORTC = 0b00000011;
   DDRD = 0xFF;
   PORTD = 0x00;
   ADC_init();
   sonar_init();
 
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
 
   tasks[5].period = 200;
   tasks[5].state = RB_start;
   tasks[5].elapsedTime = tasks[5].period;
   tasks[5].TickFct = &RB_Tick;
 
   tasks[6].period = 250;
   tasks[6].state = BL_start;
   tasks[6].elapsedTime = tasks[6].period;
   tasks[6].TickFct = &BL_Tick;
 
   TimerSet(GCD_PERIOD);
   TimerOn();
 
   while (1) {}
   return 0;
 }