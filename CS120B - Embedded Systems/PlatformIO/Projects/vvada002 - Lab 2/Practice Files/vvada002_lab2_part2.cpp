/*        Kaushik Vada & vvada002@ucr.edu:

*          Discussion Section: 021

*         Assignment: Lab #2  Exercise #2

*         Exercise Description: [count in binary from 0–15 using 2 buttons and 4 LEDs]

*         I acknowledge all content contained herein, excluding template or example code, is my own original work.

*         Demo Link:

*/

#include <avr/io.h>
#include <util/delay.h>

int main(void)
{
    // Set buttons (PC1 = LEFT, PC0 = RIGHT) as inputs with pull-ups
    DDRC = DDRC & ~((1 << PC1) | (1 << PC0));  
    PORTC = PORTC | ((1 << PC1) | (1 << PC0)); 

    // Set LEDs (PD5..PD2) as outputs and turn them off
    DDRD = DDRD | 0b00111100;     
    PORTD = PORTD & ~0b00111100;  

    unsigned char counter_value = 0; // Counter for 0–15

    unsigned char prev_left = 1;  // Last state of LEFT button
    unsigned char prev_right = 1; // Last state of RIGHT button

    while (1)
    {
        // Read button states (pressed = 0)
        unsigned char curr_left = (PINC & (1 << PC1)) == 0 ? 0 : 1;
        unsigned char curr_right = (PINC & (1 << PC0)) == 0 ? 0 : 1;

        // If LEFT button is pressed, increment the counter
        if (curr_left == 0 && prev_left == 1) {
            counter_value = counter_value + 1;
            if (counter_value > 15) {
                counter_value = 0; // Wrap around to 0
            }
        }

        // If RIGHT button is pressed, decrement the counter
        if (curr_right == 0 && prev_right == 1) {
            if (counter_value == 0) {
                counter_value = 15; // Wrap around to 15
            } else {
                counter_value = counter_value - 1;
            }
        }

        // Update previous button states
        prev_left = curr_left;
        prev_right = curr_right;

        // Display the counter value on LEDs (PD5..PD2)
        PORTD = (PORTD & 0b11000011) | ((counter_value & 0x0F) << 2);

        _delay_ms(50); // Debounce delay
    }
}