/*        Kaushik Vada & vvada002@ucr.edu:

*          Discussion Section: 021

*         Assignment: Lab #2  Exercise #1

*         Exercise Description: [move a binary LED left and right using 2 buttons]

*         I acknowledge all content contained herein, excluding template or example code, is my own original work.

*         Demo Link:

*/

#include <avr/io.h>
#include <util/delay.h>

int main(void)
{
    // Set LEDs (PD5–PD2) as outputs
    DDRD |= 0b00111100;    
    PORTD &= ~0b00111100;  // Turn the LEDs off

    // Set buttons (PC0, PC1) as inputs
    DDRC &= ~((1 << PC0) | (1 << PC1));   
    PORTC |= (1 << PC0) | (1 << PC1);    

    int state = 0;  // Tracks which LED is on at the moment

    // Turn on the first LED (PD5)
    PORTD = (1 << (5 - state));   

    int prev_left = 1;  // Previous state of left button
    int prev_right = 1; // Previous state of right button

    while (1)
    {
        // get the current state of the button
        int curr_left = !(PINC & (1 << PC1));  
        int curr_right = !(PINC & (1 << PC0));  

        // Move LED left if left button is pressed
        if (curr_left && !prev_left && state > 0)
        {
            state--;
            PORTD = (1 << (5 - state));  
        }

        // Move LED right if right button is pressed
        if (curr_right && !prev_right && state < 3)
        {
            state++;
            PORTD = (1 << (5 - state));
        }

        // Update button states
        prev_left = curr_left;
        prev_right = curr_right;

        _delay_ms(50);  // Software Delay
    }
}