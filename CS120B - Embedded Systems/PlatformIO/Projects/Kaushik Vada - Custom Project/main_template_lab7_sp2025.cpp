/*#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include "LCD.h"
#include "timerISR.h"
#include "periph.h" // Include periph.h to use ADC_init and ADC_read

// Define constants for the game
#define NUM_TASKS 4 // Number of tasks being used
#define TASK1_PERIOD 100 // Game control task period
#define TASK2_PERIOD 1000 // Obstacle generation task period
#define TASK3_PERIOD 50 // Player input task period
#define TASK4_PERIOD 100 // Display update task period
#define GCD_PERIOD 50 // GCD period in milliseconds
#define MAX_SCORE 10 // Maximum score to win the game
#define MAX_LIVES 3 // Maximum lives before game over

// Pin definitions
#define LED_LEFT PD4
#define LED_MIDDLE PD5
#define LED_RIGHT PD6
#define BUTTON_LEFT PC2 // A2
#define BUTTON_MIDDLE PC1 // A1
#define BUTTON_RIGHT PC0 // A0
#define BUZZER PD2

// Task struct for concurrent state machine implementations
typedef struct _task {
    signed char state; // Task's current state
    unsigned long period; // Task period
    unsigned long elapsedTime; // Time elapsed since last task tick
    int (*TickFct)(int); // Task tick function
} task;

task tasks[NUM_TASKS]; // Declare task array

// Game variables
unsigned char score = 0; // Player's score
unsigned char lives = MAX_LIVES; // Player's remaining lives
unsigned char obstacle = 0; // Current obstacle (0 = none, 1 = left, 2 = center, 3 = right)
unsigned char buttonPressed = 0; // Button pressed by the player
unsigned char gameRunning = 0; // Game running flag

// Function prototypes
void TimerSet(unsigned long); // Set timer period
void TimerOn(); // Start timer
void TimerISR(void); // Timer interrupt service routine (updated to match timerISR.h)
void playTone(unsigned char); // Play a tone on the buzzer
void resetGame(); // Reset the game

// Tick function for Game Control
enum GameControlStates { GC_START, GC_WAIT, GC_WIN, GC_LOSE } gameControlState;
int Tick_GameControl(int state) {
    switch (state) {
        case GC_START:
            resetGame(); // Reset game variables
            lcd_clear(); // Clear the LCD screen
            lcd_write_str("Press Start"); // Display "Press Start"
            if (buttonPressed == 1) { // Start button pressed
                gameRunning = 1;
                state = GC_WAIT;
                buttonPressed = 0;
            }
            break;

        case GC_WAIT:
            if (score >= MAX_SCORE) { // Check for win condition
                state = GC_WIN;
            } else if (lives == 0) { // Check for lose condition
                state = GC_LOSE;
            }
            break;

        case GC_WIN:
            gameRunning = 0;
            lcd_clear(); // Clear the LCD screen
            lcd_write_str("Victory!"); // Display "Victory!"
            _delay_ms(2000);
            state = GC_START;
            break;

        case GC_LOSE:
            gameRunning = 0;
            lcd_clear(); // Clear the LCD screen
            lcd_write_str("Game Over"); // Display "Game Over"
            _delay_ms(2000);
            state = GC_START;
            break;

        default:
            state = GC_START;
            break;
    }
    return state;
}

// Tick function for Obstacle Generation
enum ObstacleGenStates { OG_WAIT, OG_GENERATE } obstacleGenState;
int Tick_ObstacleGen(int state) {
    switch (state) {
        case OG_WAIT:
            if (gameRunning) {
                state = OG_GENERATE;
            }
            break;

        case OG_GENERATE:
            obstacle = (rand() % 3) + 1; // Randomly select an obstacle
            state = OG_WAIT;
            break;

        default:
            state = OG_WAIT;
            break;
    }
    return state;
}

// Tick function for Player Input
enum PlayerInputStates { PI_WAIT, PI_CHECK } playerInputState;
int Tick_PlayerInput(int state) {
    switch (state) {
        case PI_WAIT:
            if (gameRunning) {
                if (!(PINC & (1 << BUTTON_LEFT))) { // Left button pressed
                    buttonPressed = 1;
                } else if (!(PINC & (1 << BUTTON_MIDDLE))) { // Middle button pressed
                    buttonPressed = 2;
                } else if (!(PINC & (1 << BUTTON_RIGHT))) { // Right button pressed
                    buttonPressed = 3;
                }
                if (buttonPressed) {
                    state = PI_CHECK;
                }
            }
            break;

        case PI_CHECK:
            if (buttonPressed == obstacle) { // Correct button pressed
                score++;
                playTone(1); // Play success tone
            } else { // Incorrect button or no response
                lives--;
                playTone(0); // Play failure tone
            }
            buttonPressed = 0; // Reset button press
            state = PI_WAIT;
            break;

        default:
            state = PI_WAIT;
            break;
    }
    return state;
}

// Tick function for Display Update
enum DisplayUpdateStates { DU_UPDATE } displayUpdateState;
int Tick_DisplayUpdate(int state) {
    switch (state) {
        case DU_UPDATE:
            if (gameRunning) {
                lcd_clear(); // Clear the LCD screen
                lcd_write_str("Score: "); // Ensure const char* is used
                lcd_write_character(score + '0'); // Display score
                lcd_goto_xy(1, 0); // Corrected to handle line and position
                lcd_write_str("Lives: "); // Ensure const char* is used
                lcd_write_character(lives + '0'); // Display lives
            }
            break;

        default:
            state = DU_UPDATE;
            break;
    }
    return state;
}

int main(void) {
    // Initialize inputs and outputs
    ADC_init(); // Use ADC_init from periph.h
    DDRB = 0xFF; PORTB = 0x00; // Set PORTB as output for LEDs and buzzer
    DDRC = 0x00; PORTC = 0xFF; // Set PORTC as input for buttons (pull-up resistors enabled)
    DDRD = (1 << LED_LEFT) | (1 << LED_MIDDLE) | (1 << LED_RIGHT) | (1 << BUZZER); // Set LEDs and buzzer as outputs
    PORTD = 0x00; // Initialize PORTD outputs to low

    lcd_init(); // Initialize the LCD

    // Initialize tasks
    tasks[0].period = TASK1_PERIOD;
    tasks[0].state = GC_START;
    tasks[0].elapsedTime = 0;
    tasks[0].TickFct = &Tick_GameControl;

    tasks[1].period = TASK2_PERIOD;
    tasks[1].state = OG_WAIT;
    tasks[1].elapsedTime = 0;
    tasks[1].TickFct = &Tick_ObstacleGen;

    tasks[2].period = TASK3_PERIOD;
    tasks[2].state = PI_WAIT;
    tasks[2].elapsedTime = 0;
    tasks[2].TickFct = &Tick_PlayerInput;

    tasks[3].period = TASK4_PERIOD;
    tasks[3].state = DU_UPDATE;
    tasks[3].elapsedTime = 0;
    tasks[3].TickFct = &Tick_DisplayUpdate;

    TimerSet(GCD_PERIOD); // Set timer period
    TimerOn(); // Start timer

    while (1) {
        // Main loop does nothing, tasks are handled in TimerISR
    }

    return 0;
}

// Timer interrupt service routine
void TimerISR(void) { // Updated to match timerISR.h
    for (unsigned int i = 0; i < NUM_TASKS; i++) {
        if (tasks[i].elapsedTime >= tasks[i].period) {
            tasks[i].state = tasks[i].TickFct(tasks[i].state); // Execute task tick function
            tasks[i].elapsedTime = 0; // Reset elapsed time
        }
        tasks[i].elapsedTime += GCD_PERIOD; // Increment elapsed time
    }
}

// Play a tone on the buzzer
void playTone(unsigned char success) {
    if (success) {
        // Play success tone
        PORTD |= (1 << BUZZER);
        _delay_ms(100);
        PORTD &= ~(1 << BUZZER);
    } else {
        // Play failure tone
        PORTD |= (1 << BUZZER);
        _delay_ms(300);
        PORTD &= ~(1 << BUZZER);
    }
}

// Reset the game
void resetGame() {
    score = 0; // Reset score
    lives = MAX_LIVES; // Reset lives
    obstacle = 0; // Reset obstacle
    buttonPressed = 0; // Reset button press
    gameRunning = 0; // Reset game running flag
} */


/*#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include "LCD.h"
#include "timerISR.h"
#include "periph.h" // Include periph.h to use ADC_init and ADC_read

// Define constants for the game
#define NUM_TASKS 4 // Number of tasks being used
#define TASK1_PERIOD 100 // Game control task period
#define TASK2_PERIOD 1000 // Obstacle generation task period
#define TASK3_PERIOD 50 // Player input task period
#define TASK4_PERIOD 100 // Display update task period
#define GCD_PERIOD 50 // GCD period in milliseconds
#define MAX_SCORE 10 // Maximum score to win the game
#define MAX_LIVES 3 // Maximum lives before game over

// --- PIN MAP (corrected) ---
// LEDs on PD4–PD6
#define LED_LEFT   PD4   // Left LED   -> PD4
#define LED_MIDDLE PD5   // Middle LED -> PD5
#define LED_RIGHT  PD6   // Right LED  -> PD6
// Buttons on PC0–PC2 (active‑LOW with pull‑ups)
#define BUTTON_RIGHT  PC0  // Right button  -> PC0 (A0)
#define BUTTON_MIDDLE PC1  // Middle button -> PC1 (A1)
#define BUTTON_LEFT   PC2  // Left button   -> PC2 (A2)
#define BUZZER        PD2

// Task struct for concurrent state machine implementations
typedef struct _task {
    signed char state; // Task's current state
    unsigned long period; // Task period
    unsigned long elapsedTime; // Time elapsed since last task tick
    int (*TickFct)(int); // Task tick function
} task;

task tasks[NUM_TASKS]; // Declare task array

// Game variables
unsigned char score = 0; // Player's score
unsigned char lives = MAX_LIVES; // Player's remaining lives
unsigned char obstacle = 0; // Current obstacle (0 = none, 1 = left, 2 = center, 3 = right)
unsigned char buttonPressed = 0; // Button pressed by the player
unsigned char gameRunning = 0; // Game running flag

// Add new state tracking variables
unsigned char inputHandled = 0;    // Track if current obstacle has been handled
unsigned char activeWindow = 0;    // Track if input window is active
unsigned char inputTimer = 0; // Timer to track the input window
unsigned char delayTimer = 0; // Timer to wait before starting the input window

// Function prototypes
void TimerSet(unsigned long); // Set timer period
void TimerOn(); // Start timer
void TimerISR(void); // Timer interrupt service routine (updated to match timerISR.h)
void playTone(unsigned char); // Play a tone on the buzzer
void resetGame(); // Reset the game

// Tick function for Game Control
enum GameControlStates { GC_START, GC_WAIT, GC_WIN, GC_LOSE } gameControlState;
int Tick_GameControl(int state) {
    switch (state) {
        case GC_START:
            resetGame(); // Reset game variables
            lcd_clear(); // Clear the LCD screen
            lcd_write_str("Press Start"); // Display "Press Start"
            if (!(PINC & (1 << BUTTON_LEFT))) { // Check if "Start" button (LEFT) is pressed
                _delay_ms(50); // Debounce delay
                if (!(PINC & (1 << BUTTON_LEFT))) { // Confirm button press
                    gameRunning = 1;
                    state = GC_WAIT;
                }
            }
            break;

        case GC_WAIT:
            if (score >= MAX_SCORE) { // Check for win condition
                state = GC_WIN;
            } else if (lives == 0) { // Check for lose condition
                state = GC_LOSE;
            }
            break;

        case GC_WIN:
            gameRunning = 0;
            lcd_clear(); // Clear the LCD screen
            lcd_write_str("Victory!"); // Display "Victory!"
            _delay_ms(2000);
            state = GC_START;
            break;

        case GC_LOSE:
            gameRunning = 0;
            lcd_clear(); // Clear the LCD screen
            lcd_write_str("Game Over"); // Display "Game Over"
            _delay_ms(2000);
            state = GC_START;
            break;

        default:
            state = GC_START;
            break;
    }
    return state;
}

// Tick function for Obstacle Generation
enum ObstacleGenStates { OG_WAIT, OG_GENERATE } obstacleGenState;
int Tick_ObstacleGen(int state) {
    switch (state) {
        case OG_WAIT:
            if (gameRunning && obstacle == 0 && !activeWindow) {
                state = OG_GENERATE;
            }
            break;

        case OG_GENERATE:
            obstacle = (rand() % 3) + 1; // Randomly select an obstacle
            PORTD &= ~((1 << LED_LEFT) | (1 << LED_MIDDLE) | (1 << LED_RIGHT)); // Turn off all LEDs
            if (obstacle == 1) PORTD |= (1 << LED_LEFT); // Turn on LEFT LED
            else if (obstacle == 2) PORTD |= (1 << LED_MIDDLE); // Turn on MIDDLE LED
            else if (obstacle == 3) PORTD |= (1 << LED_RIGHT); // Turn on RIGHT LED
            delayTimer = 30; // Set delay timer (1.5 seconds = 30 ticks at 50ms GCD)
            activeWindow = 0; // Input window is not yet active
            inputHandled = 0; // Reset input handled flag
            state = OG_WAIT;
            break;

        default:
            state = OG_WAIT;
            break;
    }
    return state;
}

// Tick function for Player Input
enum PlayerInputStates { PI_WAIT, PI_CHECK } playerInputState;

int Tick_PlayerInput(int state) {
    switch (state) {
        case PI_WAIT:
            if (!gameRunning || inputHandled) {
                break; // Do nothing if the game is not running or input is already handled
            }

            if (delayTimer > 0) {
                delayTimer--; // Wait for the delay to finish
                break;
            }

            if (!activeWindow) {
                activeWindow = 1; // Activate the input window after the delay
                inputTimer = 20; // Set input timer (1 second = 20 ticks at 50ms GCD)
            }

            if (inputTimer > 0) { // Check for button presses during the active window
                if (!(PINC & (1 << BUTTON_LEFT))) { // Check if LEFT button (PC2) is pressed
                    _delay_ms(50); // Debounce delay
                    if (!(PINC & (1 << BUTTON_LEFT))) buttonPressed = 1; // Confirm button press
                } else if (!(PINC & (1 << BUTTON_MIDDLE))) { // Check if MIDDLE button (PC1) is pressed
                    _delay_ms(50); // Debounce delay
                    if (!(PINC & (1 << BUTTON_MIDDLE))) buttonPressed = 2; // Confirm button press
                } else if (!(PINC & (1 << BUTTON_RIGHT))) { // Check if RIGHT button (PC0) is pressed
                    _delay_ms(50); // Debounce delay
                    if (!(PINC & (1 << BUTTON_RIGHT))) buttonPressed = 3; // Confirm button press
                }

                if (buttonPressed) {
                    state = PI_CHECK; // Transition to check state
                }
            } else if (!inputHandled) { // Timer expired without input
                lives--; // Decrement lives for timeout
                playTone(0); // Play failure tone
                inputHandled = 1; // Mark input as handled
                activeWindow = 0; // Close input window
                obstacle = 0; // Reset obstacle
                PORTD &= ~((1 << LED_LEFT) | (1 << LED_MIDDLE) | (1 << LED_RIGHT)); // Turn off all LEDs
            }
            break;

        case PI_CHECK:
            if (!inputHandled) { // Process input only if it hasn't been handled yet
                if (buttonPressed == obstacle) { // Correct button pressed
                    score++; // Increment score
                    playTone(1); // Play success tone
                } else { // Incorrect button pressed
                    lives--; // Decrement lives
                    playTone(0); // Play failure tone
                }
                inputHandled = 1; // Mark input as handled
            }
            activeWindow = 0; // Close input window
            obstacle = 0; // Reset obstacle
            buttonPressed = 0; // Reset button press
            PORTD &= ~((1 << LED_LEFT) | (1 << LED_MIDDLE) | (1 << LED_RIGHT)); // Turn off all LEDs
            state = PI_WAIT;
            break;

        default:
            state = PI_WAIT;
            break;
    }
    return state;
}

// Tick function for Display Update
enum DisplayUpdateStates { DU_UPDATE } displayUpdateState;
int Tick_DisplayUpdate(int state) {
    switch (state) {
        case DU_UPDATE:
            if (gameRunning) {
                lcd_clear(); // Clear the LCD screen
                lcd_write_str("Score: "); // Display "Score: "
                lcd_write_character(score + '0'); // Display score
                lcd_goto_xy(1, 0); // Move to the second line
                lcd_write_str("Lives: "); // Display "Lives: "
                lcd_write_character(lives + '0'); // Display lives
            }
            break;

        default:
            state = DU_UPDATE;
            break;
    }
    return state;
}

int main(void) {
    // Initialize inputs and outputs
    ADC_init(); // Use ADC_init from periph.h

    // Buttons: PC0–PC2 inputs with pull‑ups
    DDRC  &= ~((1<<BUTTON_LEFT)|(1<<BUTTON_MIDDLE)|(1<<BUTTON_RIGHT));
    PORTC |=  ((1<<BUTTON_LEFT)|(1<<BUTTON_MIDDLE)|(1<<BUTTON_RIGHT));

    // LEDs + Buzzer: PD2, PD4–PD6 outputs
    DDRD |= (1<<BUZZER)|(1<<LED_LEFT)|(1<<LED_MIDDLE)|(1<<LED_RIGHT);
    PORTD &= ~((1<<LED_LEFT)|(1<<LED_MIDDLE)|(1<<LED_RIGHT));   // LEDs off

    lcd_init(); // Initialize the LCD

    // Initialize tasks
    tasks[0].period = TASK1_PERIOD;
    tasks[0].state = GC_START;
    tasks[0].elapsedTime = 0;
    tasks[0].TickFct = &Tick_GameControl;

    tasks[1].period = TASK2_PERIOD;
    tasks[1].state = OG_WAIT;
    tasks[1].elapsedTime = 0;
    tasks[1].TickFct = &Tick_ObstacleGen;

    tasks[2].period = TASK3_PERIOD;
    tasks[2].state = PI_WAIT;
    tasks[2].elapsedTime = 0;
    tasks[2].TickFct = &Tick_PlayerInput;

    tasks[3].period = TASK4_PERIOD;
    tasks[3].state = DU_UPDATE;
    tasks[3].elapsedTime = 0;
    tasks[3].TickFct = &Tick_DisplayUpdate;

    TimerSet(GCD_PERIOD); // Set timer period
    TimerOn(); // Start timer

    while (1) {
        // Main loop does nothing, tasks are handled in TimerISR
    }

    return 0;
}

// Timer interrupt service routine
void TimerISR(void) {
    for (unsigned int i = 0; i < NUM_TASKS; i++) {
        if (tasks[i].elapsedTime >= tasks[i].period) {
            tasks[i].state = tasks[i].TickFct(tasks[i].state);
            tasks[i].elapsedTime = 0;
        }
        tasks[i].elapsedTime += GCD_PERIOD;
    }

    if (inputTimer > 0 && activeWindow) {
        inputTimer--; // Decrement input timer
        if (inputTimer == 0 && !inputHandled) { // Time window expired without input
            lives--; // Decrement lives for timeout
            playTone(0); // Play failure tone
            inputHandled = 1; // Mark input as handled
            activeWindow = 0; // Close input window
            inputTimer = 0;
            obstacle = 0; // Reset obstacle
            PORTD &= ~((1 << LED_LEFT) | (1 << LED_MIDDLE) | (1 << LED_RIGHT)); // Turn off all LEDs
        }
    }
}

// Play a tone on the buzzer
void playTone(unsigned char success) {
    if (success) {
        // Play success tone
        PORTD |= (1 << BUZZER);
        _delay_ms(100);
        PORTD &= ~(1 << BUZZER);
    } else {
        // Play failure tone
        PORTD |= (1 << BUZZER);
        _delay_ms(300);
        PORTD &= ~(1 << BUZZER);
    }
}

// Reset the game
void resetGame() {
    score = 0; // Reset score
    lives = MAX_LIVES; // Reset lives
    obstacle = 0; // Reset obstacle
    buttonPressed = 0; // Reset button press
    gameRunning = 0; // Reset game running flag
    inputHandled = 0; // Reset input handled flag
    activeWindow = 0; // Reset input window
    inputTimer = 0; // Reset input timer
    delayTimer = 0; // Reset delay timer
    PORTD &= ~((1 << LED_LEFT) | (1 << LED_MIDDLE) | (1 << LED_RIGHT)); // Turn off all LEDs
}*/


/*

#include <avr/io.h> // AVR I/O library for port and pin manipulation
#include <avr/interrupt.h> // AVR interrupt library
#include <util/delay.h> // AVR delay library for timing
#include <stdlib.h> // Standard library for random number generation
#include <stdio.h> // Standard I/O library for debugging
#include "LCD.h" // Custom library for LCD control
#include "timerISR.h" // Custom library for timer interrupt service routines
#include "periph.h" // Custom library for peripheral initialization (e.g., ADC)

// Define constants for the game
#define NUM_TASKS 4 // Number of tasks being used
#define TASK1_PERIOD 100 // Game control task period in milliseconds
#define TASK2_PERIOD 1000 // Obstacle generation task period in milliseconds
#define TASK3_PERIOD 50 // Player input task period in milliseconds
#define TASK4_PERIOD 100 // Display update task period in milliseconds
#define GCD_PERIOD 50 // GCD period in milliseconds (smallest common period for tasks)
#define MAX_SCORE 10 // Maximum score to win the game
#define MAX_LIVES 3 // Maximum lives before game over

// Pin definitions
#define LED_LEFT PD4 // Pin for the left LED
#define LED_MIDDLE PD5 // Pin for the middle LED
#define LED_RIGHT PD6 // Pin for the right LED
#define BUZZER PD2 // Pin for the buzzer
#define BUTTON_LEFT PC2 // Pin for the left button
#define BUTTON_MIDDLE PC1 // Pin for the middle button
#define BUTTON_RIGHT PC0 // Pin for the right button

// Task struct for concurrent state machine implementations
typedef struct _task {
    signed char state; // Task's current state
    unsigned long period; // Task period in milliseconds
    unsigned long elapsedTime; // Time elapsed since last task tick
    int (*TickFct)(int); // Pointer to the task's tick function
} task;

task tasks[NUM_TASKS]; // Declare an array of tasks

// Game variables
unsigned char score = 0; // Player's score
unsigned char lives = MAX_LIVES; // Player's remaining lives
unsigned char obstacle = 0; // Current obstacle (0 = none, 1 = left, 2 = center, 3 = right)
unsigned char buttonPressed = 0; // Button pressed by the player
unsigned char gameRunning = 0; // Game running flag (1 = running, 0 = stopped)

// Function prototypes
void TimerSet(unsigned long); // Set the timer period
void TimerOn(); // Start the timer
void TimerISR(void); // Timer interrupt service routine
void playTone(unsigned char); // Play a tone on the buzzer (success or failure)
void resetGame(); // Reset the game variables and state
void debugDisplay(); // Display debugging information on the LCD

// Tick function for Game Control
enum GameControlStates { GC_START, GC_WAIT, GC_WIN, GC_LOSE } gameControlState; // States for game control
int Tick_GameControl(int state) {
    switch (state) {
        case GC_START: // Initial state
            resetGame(); // Reset game variables
            gameRunning = 1; // Start the game
            state = GC_WAIT; // Transition to the game wait state
            break;

        case GC_WAIT: // Game running state
            if (score >= MAX_SCORE) { // Check if the player has won
                state = GC_WIN; // Transition to the win state
            } else if (lives == 0) { // Check if the player has lost
                state = GC_LOSE; // Transition to the lose state
            }
            break;

        case GC_WIN: // Win state
            gameRunning = 0; // Stop the game
            lcd_clear(); // Clear the LCD screen
            lcd_write_str("Victory!"); // Display "Victory!" on the LCD
            _delay_ms(2000); // Wait for 2 seconds
            state = GC_START; // Reset to the start state
            break;

        case GC_LOSE: // Lose state
            gameRunning = 0; // Stop the game
            lcd_clear(); // Clear the LCD screen
            lcd_write_str("Game Over :(("); // Display "Game Over" on the LCD
            _delay_ms(2000); // Wait for 2 seconds
            state = GC_START; // Reset to the start state
            break;

        default: // Default state
            state = GC_START; // Reset to the start state
            break;
    }
    return state; // Return the next state
}

// Tick function for Obstacle Generation
enum ObstacleGenStates { OG_WAIT, OG_GENERATE } obstacleGenState; // States for obstacle generation
int Tick_ObstacleGen(int state) {
    switch (state) {
        case OG_WAIT: // Waiting for the game to start
            if (gameRunning && obstacle == 0) { // If the game is running and no obstacle is active
                state = OG_GENERATE; // Transition to the generate state
            }
            break;

        case OG_GENERATE: // Generate a new obstacle
            obstacle = (rand() % 3) + 1; // Randomly select an obstacle (1 = left, 2 = middle, 3 = right)
            PORTD &= ~((1 << LED_LEFT) | (1 << LED_MIDDLE) | (1 << LED_RIGHT)); // Turn off all LEDs
            if (obstacle == 1) PORTD |= (1 << LED_LEFT); // Turn on the left LED
            else if (obstacle == 2) PORTD |= (1 << LED_MIDDLE); // Turn on the middle LED
            else if (obstacle == 3) PORTD |= (1 << LED_RIGHT); // Turn on the right LED
            debugDisplay(); // Display debugging information on the LCD
            state = OG_WAIT; // Transition back to the wait state
            break;

        default: // Default state
            state = OG_WAIT; // Reset to the wait state
            break;
    }
    return state; // Return the next state
}

// Tick function for Player Input
enum PlayerInputStates { PI_WAIT, PI_CHECK } playerInputState; // States for player input
int Tick_PlayerInput(int state) {
    static unsigned char inputHandled = 0; // Ensure input is handled only once per obstacle
    static unsigned char inputTimer = 20;  // Timer for input window (1 second = 20 ticks at 50ms GCD)

    switch (state) {
        case PI_WAIT: // Waiting for player input
            if (gameRunning && obstacle != 0 && !inputHandled) { // If the game is running, an obstacle is active, and input is not yet handled
                if (inputTimer > 0) { // If the input timer has not expired
                    inputTimer--; // Decrement the input timer

                    // Debugging: Display raw button states on the LCD
                    lcd_clear();
                    lcd_write_str("Raw Btn: ");
                    lcd_write_character(~PINC + '0'); // Display raw button states (inverted due to pull-ups)
                    _delay_ms(500); // Pause for readability

                    // Check for button presses
                    if (!(PINC & (1 << BUTTON_LEFT))) { // Check if LEFT button (PC2) is pressed
                        _delay_ms(50); // Debounce delay
                        if (!(PINC & (1 << BUTTON_LEFT))) buttonPressed = 1;
                    } else if (!(PINC & (1 << BUTTON_MIDDLE))) { // Check if MIDDLE button (PC1) is pressed
                        _delay_ms(50); // Debounce delay
                        if (!(PINC & (1 << BUTTON_MIDDLE))) buttonPressed = 2;
                    } else if (!(PINC & (1 << BUTTON_RIGHT))) { // Check if RIGHT button (PC0) is pressed
                        _delay_ms(50); // Debounce delay
                        if (!(PINC & (1 << BUTTON_RIGHT))) buttonPressed = 3;
                    } else {
                        buttonPressed = 0; // Reset buttonPressed if no button is pressed
                    }

                    // Debugging: Display buttonPressed value on the LCD
                    lcd_clear();
                    lcd_write_str("Btn: ");
                    lcd_write_character(buttonPressed + '0'); // Display the current buttonPressed value
                    _delay_ms(500); // Pause for readability

                    if (buttonPressed) { // If a button is pressed
                        state = PI_CHECK; // Transition to the check state
                    }
                } else { // If the input timer has expired
                    if (!inputHandled) { // If input has not yet been handled
                        lives--; // Decrement lives for timeout
                        playTone(0); // Play failure tone
                        inputHandled = 1; // Mark input as handled
                        obstacle = 0; // Reset obstacle
                        PORTD &= ~((1 << LED_LEFT) | (1 << LED_MIDDLE) | (1 << LED_RIGHT)); // Turn off all LEDs
                        inputTimer = 20; // Reset input timer
                    }
                }
            }
            break;

        case PI_CHECK: // Checking player input
            if (buttonPressed == obstacle) { // If the correct button was pressed
                score++; // Increment score
                playTone(1); // Play success tone
            } else { // If the wrong button was pressed
                lives--; // Decrement lives
                playTone(0); // Play failure tone
            }
            inputHandled = 1; // Mark input as handled
            buttonPressed = 0; // Reset button press
            obstacle = 0; // Reset obstacle
            PORTD &= ~((1 << LED_LEFT) | (1 << LED_MIDDLE) | (1 << LED_RIGHT)); // Turn off all LEDs
            inputTimer = 20; // Reset input timer
            state = PI_WAIT; // Transition back to the wait state
            break;

        default: // Default state
            state = PI_WAIT; // Reset to the wait state
            break;
    }

    // Reset inputHandled when obstacle is reset
    if (obstacle == 0) {
        inputHandled = 0;
    }

    return state; // Return the next state
}

// Tick function for Display Update
enum DisplayUpdateStates { DU_UPDATE } displayUpdateState; // States for display update
int Tick_DisplayUpdate(int state) {
    switch (state) {
        case DU_UPDATE: // Update the display
            if (gameRunning) { // If the game is running
                lcd_clear(); // Clear the LCD screen
                lcd_write_str("Score: "); // Display "Score: "
                lcd_write_character(score + '0'); // Display the current score
                lcd_goto_xy(1, 0); // Move to the second line
                lcd_write_str("Lives: "); // Display "Lives: "
                lcd_write_character(lives + '0'); // Display the remaining lives
            }
            break;

        default: // Default state
            state = DU_UPDATE; // Stay in the update state
            break;
    }
    return state; // Return the next state
}

// Main function
int main(void) {
    // Initialize inputs and outputs
    ADC_init(); // Initialize ADC (Analog-to-Digital Converter)

    // Configure buttons as inputs with pull-up resistors
    DDRC &= ~((1 << BUTTON_LEFT) | (1 << BUTTON_MIDDLE) | (1 << BUTTON_RIGHT)); // Set PC2, PC1, PC0 as inputs
    PORTC |= (1 << BUTTON_LEFT) | (1 << BUTTON_MIDDLE) | (1 << BUTTON_RIGHT);  // Enable pull-up resistors on PC2, PC1, PC0

    // Configure LEDs and buzzer as outputs
    DDRD |= (1 << LED_LEFT) | (1 << LED_MIDDLE) | (1 << LED_RIGHT) | (1 << BUZZER); // Set PD4, PD5, PD6, PD2 as outputs
    PORTD &= ~((1 << LED_LEFT) | (1 << LED_MIDDLE) | (1 << LED_RIGHT)); // Turn off all LEDs

    lcd_init(); // Initialize the LCD

    // Initialize tasks
    tasks[0].period = TASK1_PERIOD; // Set the period for the game control task
    tasks[0].state = GC_START; // Set the initial state for the game control task
    tasks[0].elapsedTime = 0; // Initialize elapsed time for the game control task
    tasks[0].TickFct = &Tick_GameControl; // Set the tick function for the game control task

    tasks[1].period = TASK2_PERIOD; // Set the period for the obstacle generation task
    tasks[1].state = OG_WAIT; // Set the initial state for the obstacle generation task
    tasks[1].elapsedTime = 0; // Initialize elapsed time for the obstacle generation task
    tasks[1].TickFct = &Tick_ObstacleGen; // Set the tick function for the obstacle generation task

    tasks[2].period = TASK3_PERIOD; // Set the period for the player input task
    tasks[2].state = PI_WAIT; // Set the initial state for the player input task
    tasks[2].elapsedTime = 0; // Initialize elapsed time for the player input task
    tasks[2].TickFct = &Tick_PlayerInput; // Set the tick function for the player input task

    tasks[3].period = TASK4_PERIOD; // Set the period for the display update task
    tasks[3].state = DU_UPDATE; // Set the initial state for the display update task
    tasks[3].elapsedTime = 0; // Initialize elapsed time for the display update task
    tasks[3].TickFct = &Tick_DisplayUpdate; // Set the tick function for the display update task

    TimerSet(GCD_PERIOD); // Set the timer period to the GCD period
    TimerOn(); // Start the timer

    while (1) {
        // Main loop does nothing, tasks are handled in TimerISR
    }

    return 0; // Return 0 (never reached)
}

// Timer interrupt service routine
void TimerISR(void) {
    for (unsigned int i = 0; i < NUM_TASKS; i++) { // Loop through all tasks
        if (tasks[i].elapsedTime >= tasks[i].period) { // If the task's period has elapsed
            tasks[i].state = tasks[i].TickFct(tasks[i].state); // Execute the task's tick function
            tasks[i].elapsedTime = 0; // Reset the task's elapsed time
        }
        tasks[i].elapsedTime += GCD_PERIOD; // Increment the task's elapsed time
    }
}

// Play a tone on the buzzer
void playTone(unsigned char success) {
    if (success) { // If success tone
        PORTD |= (1 << BUZZER); // Turn on the buzzer
        _delay_ms(100); // Wait for 100ms
        PORTD &= ~(1 << BUZZER); // Turn off the buzzer
    } else { // If failure tone
        PORTD |= (1 << BUZZER); // Turn on the buzzer
        _delay_ms(300); // Wait for 300ms
        PORTD &= ~(1 << BUZZER); // Turn off the buzzer
    }
}

// Reset the game
void resetGame() {
    score = 0; // Reset the score
    lives = MAX_LIVES; // Reset the lives
    obstacle = 0; // Reset the obstacle
    buttonPressed = 0; // Reset the button press
    gameRunning = 0; // Reset the game running flag
    PORTD &= ~((1 << LED_LEFT) | (1 << LED_MIDDLE) | (1 << LED_RIGHT)); // Turn off all LEDs
}

// Debugging function to display internal variables on the LCD
void debugDisplay() {
    lcd_clear(); // Clear the LCD screen
    lcd_write_str("Obs: "); // Display "Obs: "
    lcd_write_character(obstacle + '0'); // Display the current obstacle
    lcd_goto_xy(1, 0); // Move to the second line
    lcd_write_str("Btn: "); // Display "Btn: "
    lcd_write_character(buttonPressed + '0'); // Display the current button pressed
    _delay_ms(1000); // Wait for 1 second
} */

/*
#include <avr/io.h> // AVR I/O library for port and pin manipulation
#include <avr/interrupt.h> // AVR interrupt library
#include <util/delay.h> // AVR delay library for timing
#include <stdlib.h> // Standard library for random number generation
#include <stdio.h> // Standard I/O library for debugging
#include "LCD.h" // Custom library for LCD control
#include "timerISR.h" // Custom library for timer interrupt service routines
#include "periph.h" // Custom library for peripheral initialization (e.g., ADC)
#include "irAVR.h" // Include the IR remote library

// Define constants for the game
#define NUM_TASKS 4 // Number of tasks being used
#define TASK1_PERIOD 100 // Game control task period in milliseconds
#define TASK2_PERIOD 1000 // Obstacle generation task period in milliseconds
#define TASK3_PERIOD 50 // Player input task period in milliseconds
#define TASK4_PERIOD 100 // Display update task period in milliseconds
#define GCD_PERIOD 50 // GCD period in milliseconds (smallest common period for tasks)
#define MAX_SCORE 10 // Maximum score to win the game
#define MAX_LIVES 3 // Maximum lives before game over

// Pin definitions
#define LED_LEFT PD4 // Pin for the left LED
#define LED_MIDDLE PD5 // Pin for the middle LED
#define LED_RIGHT PD6 // Pin for the right LED
#define BUZZER PD2 // Pin for the buzzer
#define IR_PIN PD3 // Pin for the IR remote

// Task struct for concurrent state machine implementations
typedef struct _task {
    signed char state; // Task's current state
    unsigned long period; // Task period in milliseconds
    unsigned long elapsedTime; // Time elapsed since last task tick
    int (*TickFct)(int); // Pointer to the task's tick function
} task;

task tasks[NUM_TASKS]; // Declare an array of tasks

// Game variables
unsigned char score = 0; // Player's score
unsigned char lives = MAX_LIVES; // Player's remaining lives
unsigned char obstacle = 0; // Current obstacle (0 = none, 1 = left, 2 = center, 3 = right)
unsigned char buttonPressed = 0; // Button pressed by the player
unsigned char gameRunning = 0; // Game running flag (1 = running, 0 = stopped)

decode_results irResults; // Variable to store IR decode results
unsigned long lastValidCode = 0; // Store the last valid button press

// Function prototypes
void TimerSet(unsigned long); // Set the timer period
void TimerOn(); // Start the timer
void TimerISR(void); // Timer interrupt service routine
void playTone(unsigned char); // Play a tone on the buzzer (success or failure)
void resetGame(); // Reset the game variables and state
void debugDisplay(); // Display debugging information on the LCD
void displayInstructions(); // Display instructions for using the IR remote
void debugIRInput(); // Display IR input on the LCD for debugging
void debugIRSerial(); // Print IR input to the serial monitor
void UART_init(unsigned long); // Initialize UART for serial communication
void UART_sendChar(char); // Send a character over UART
void UART_sendString(const char*); // Send a string over UART

// Tick function for Game Control
enum GameControlStates { GC_START, GC_WAIT, GC_WIN, GC_LOSE, GC_HOLD } gameControlState; // States for game control
int Tick_GameControl(int state) {
    static unsigned char displayTimer = 0; // Countdown timer for victory/lose message (in GCD ticks)

    switch (state) {
        case GC_START: // Initial state
            resetGame(); // Reset game variables
            displayInstructions(); // Display instructions on the LCD
            gameRunning = 1; // Start the game
            state = GC_WAIT; // Transition to the game wait state
            break;

        case GC_WAIT: // Game running state
            if (score >= MAX_SCORE) { // Check if the player has won
                gameRunning = 0;
                lcd_clear();
                lcd_write_str("Victory!");
                displayTimer = 40; // 2000 ms / 50 ms GCD = 40 ticks
                state = GC_HOLD;
            } else if (lives == 0) { // Check if the player has lost
                gameRunning = 0;
                lcd_clear();
                lcd_write_str("Game Over :((");
                displayTimer = 40;
                state = GC_HOLD;
            }
            break;

        case GC_HOLD: // Holding the victory/lose message
            if (displayTimer > 0) {
                displayTimer--;
            } else {
                state = GC_START; // After countdown, go to reset
            }
            break;

        default: // Default state
            state = GC_START; // Reset to the start state
            break;
    }
    return state; // Return the next state
}

// Tick function for Obstacle Generation
enum ObstacleGenStates { OG_WAIT, OG_GENERATE } obstacleGenState; // States for obstacle generation
int Tick_ObstacleGen(int state) {
    switch (state) {
        case OG_WAIT: // Waiting for the game to start
            if (gameRunning && obstacle == 0) { // If the game is running and no obstacle is active
                state = OG_GENERATE; // Transition to the generate state
            }
            break;

        case OG_GENERATE: // Generate a new obstacle
            obstacle = (rand() % 3) + 1; // Randomly select an obstacle (1 = left, 2 = middle, 3 = right)
            PORTD &= ~((1 << LED_LEFT) | (1 << LED_MIDDLE) | (1 << LED_RIGHT)); // Turn off all LEDs
            if (obstacle == 1) PORTD |= (1 << LED_LEFT); // Turn on the left LED
            else if (obstacle == 2) PORTD |= (1 << LED_MIDDLE); // Turn on the middle LED
            else if (obstacle == 3) PORTD |= (1 << LED_RIGHT); // Turn on the right LED
            debugDisplay(); // Display debugging information on the LCD
            state = OG_WAIT; // Transition back to the wait state
            break;

        default: // Default state
            state = OG_WAIT; // Reset to the wait state
            break;
    }
    return state; // Return the next state
}

// Tick function for Player Input
enum PlayerInputStates { PI_WAIT, PI_CHECK } playerInputState; // States for player input
int Tick_PlayerInput(int state) {
    static unsigned char inputHandled = 0; // Ensure input is handled only once per obstacle
    static unsigned char inputTimer = 20;  // Timer for input window (1 second = 20 ticks at 50ms GCD)

    switch (state) {
        case PI_WAIT: // Waiting for player input
            if (gameRunning && obstacle != 0 && !inputHandled) { // If the game is running, an obstacle is active, and input is not yet handled
                if (inputTimer > 0) { // If the input timer has not expired
                    inputTimer--; // Decrement the input timer

                    // Decode IR input
                    if (IRdecode(&irResults)) {
                        if (irResults.value == 0xFFFFFFFF) {
                            irResults.value = lastValidCode;  // Use previous valid code on repeat
                        } else {
                            lastValidCode = irResults.value;  // Update last valid code
                        }

                        switch (irResults.value) {
                            case 0xF30CFF00: // IR input "1"
                                buttonPressed = 1;
                                break;
                            case 0xE718FF00: // IR input "2"
                                buttonPressed = 2;
                                break;
                            case 0xA15EFF00: // IR input "3"
                                buttonPressed = 3;
                                break;
                            default:
                                buttonPressed = 0; // Reset buttonPressed if no valid input
                                break;
                        }
                        IRresume(); // Resume IR decoding
                    }

                    if (buttonPressed) { // If a button is pressed
                        state = PI_CHECK; // Transition to the check state
                    }
                } else { // If the input timer has expired
                    if (!inputHandled) { // If input has not yet been handled
                        lives--; // Decrement lives for timeout
                        playTone(0); // Play failure tone
                        inputHandled = 1; // Mark input as handled
                        obstacle = 0; // Reset obstacle
                        inputTimer = 20; // Reset input timer
                    }
                }
            }
            break;

        case PI_CHECK: // Checking player input
            if (buttonPressed == obstacle) { // If the correct button was pressed
                score++; // Increment score
                playTone(1); // Play success tone
            } else { // If the wrong button was pressed
                lives--; // Decrement lives
                playTone(0); // Play failure tone
            }
            inputHandled = 1; // Mark input as handled
            buttonPressed = 0; // Reset button press
            obstacle = 0; // Reset obstacle
            inputTimer = 20; // Reset input timer
            state = PI_WAIT; // Transition back to the wait state
            break;

        default: // Default state
            state = PI_WAIT; // Reset to the wait state
            break;
    }

    // Reset inputHandled when obstacle is reset
    if (obstacle == 0) {
        inputHandled = 0;
    }

    return state; // Return the next state
}

// Tick function for Display Update
enum DisplayUpdateStates { DU_UPDATE } displayUpdateState; // States for display update
int Tick_DisplayUpdate(int state) {
    switch (state) {
        case DU_UPDATE: // Update the display
            if (gameRunning) { // If the game is running
                lcd_clear(); // Clear the LCD screen
                lcd_write_str("Score: "); // Display "Score: "
                lcd_write_character(score + '0'); // Display the current score
                lcd_goto_xy(1, 0); // Move to the second line
                lcd_write_str("Lives: "); // Display "Lives: "
                lcd_write_character(lives + '0'); // Display the remaining lives
            }
            break;

        default: // Default state
            state = DU_UPDATE; // Stay in the update state
            break;
    }
    return state; // Return the next state
}

// Play a tone on the buzzer
void playTone(unsigned char success) {
    if (success) { // If success tone
        PORTD |= (1 << BUZZER); // Turn on the buzzer
        _delay_ms(100); // Wait for 100ms
        PORTD &= ~(1 << BUZZER); // Turn off the buzzer
    } else { // If failure tone
        PORTD |= (1 << BUZZER); // Turn on the buzzer
        _delay_ms(300); // Wait for 300ms
        PORTD &= ~(1 << BUZZER); // Turn off the buzzer
    }
}

// Reset the game
void resetGame() {
    score = 0; // Reset the score
    lives = MAX_LIVES; // Reset the lives
    obstacle = 0; // Reset the obstacle
    buttonPressed = 0; // Reset the button press
    gameRunning = 0; // Reset the game running flag
    PORTD &= ~((1 << LED_LEFT) | (1 << LED_MIDDLE) | (1 << LED_RIGHT)); // Turn off all LEDs
}

// Debugging function to display internal variables on the LCD
void debugDisplay() {
    lcd_clear(); // Clear the LCD screen
    lcd_write_str("Obs: "); // Display "Obs: "
    lcd_write_character(obstacle + '0'); // Display the current obstacle
    lcd_goto_xy(1, 0); // Move to the second line
    lcd_write_str("Btn: "); // Display "Btn: "
    lcd_write_character(buttonPressed + '0'); // Display the current button pressed
    _delay_ms(1000); // Wait for 1 second
}

// Debugging function to display IR input on the LCD
void debugIRInput() {
    if (IRdecode(&irResults)) {
        lcd_clear(); // Clear the LCD screen
        lcd_write_str("IR Input: "); // Display "IR Input: "
        lcd_write_character(irResults.value + '0'); // Display the decoded IR value
        _delay_ms(1000); // Wait for 1 second
        IRresume(); // Resume IR decoding
    }
}

// Debugging function to print IR input to the serial monitor
void debugIRSerial() {
    if (IRdecode(&irResults)) {
        if (irResults.value == 0xFFFFFFFF) {
            if (lastValidCode != 0) {
                char buffer[60];
                sprintf(buffer, "Button Pressed (Repeat):\n0x%08lX\n", lastValidCode);
                UART_sendString(buffer);
            } else {
                UART_sendString("Repeat received, but no valid code seen yet.\n");
            }
        } else {
            lastValidCode = irResults.value;
            char buffer[50];
            sprintf(buffer, "Button Pressed:\n0x%08lX\n", irResults.value);
            UART_sendString(buffer);
        }
        IRresume();
    }
}

// Function to display instructions for using the IR remote
void displayInstructions() {
    lcd_clear();
    lcd_write_str("IR Remote:");
    lcd_goto_xy(1, 0);
    lcd_write_str("1=Left 2=Mid 3=Right");
    _delay_ms(3000); // Display instructions for 3 seconds
}

// Function to initialize UART for serial communication
void UART_init(unsigned long baud) {
    unsigned int ubrr = F_CPU / 16 / baud - 1; // Calculate UBRR value
    UBRR0H = (unsigned char)(ubrr >> 8);      // Set baud rate high byte
    UBRR0L = (unsigned char)ubrr;            // Set baud rate low byte
    UCSR0B = (1 << TXEN0);                   // Enable transmitter
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);  // Set frame format: 8 data bits, 1 stop bit
}

// Function to send a character over UART
void UART_sendChar(char c) {
    while (!(UCSR0A & (1 << UDRE0))); // Wait for the transmit buffer to be empty
    UDR0 = c;                        // Send the character
}

// Function to send a string over UART
void UART_sendString(const char* str) {
    while (*str) {
        UART_sendChar(*str++);
    }
}

// Main function
int main(void) {
    // Initialize inputs and outputs
    ADC_init(); // Initialize ADC (Analog-to-Digital Converter)

    // Configure LEDs and buzzer as outputs
    DDRD |= (1 << LED_LEFT) | (1 << LED_MIDDLE) | (1 << LED_RIGHT) | (1 << BUZZER); // Set PD4, PD5, PD6, PD2 as outputs
    PORTD &= ~((1 << LED_LEFT) | (1 << LED_MIDDLE) | (1 << LED_RIGHT)); // Turn off all LEDs

    lcd_init(); // Initialize the LCD
    IRinit(&DDRD, &PIND, IR_PIN); // Initialize the IR remote on Pin 3

    UART_init(9600); // Initialize UART with a baud rate of 9600

    // Initialize tasks
    tasks[0].period = TASK1_PERIOD; // Set the period for the game control task
    tasks[0].state = GC_START; // Set the initial state for the game control task
    tasks[0].elapsedTime = 0; // Initialize elapsed time for the game control task
    tasks[0].TickFct = &Tick_GameControl; // Set the tick function for the game control task

    tasks[1].period = TASK2_PERIOD; // Set the period for the obstacle generation task
    tasks[1].state = OG_WAIT; // Set the initial state for the obstacle generation task
    tasks[1].elapsedTime = 0; // Initialize elapsed time for the obstacle generation task
    tasks[1].TickFct = &Tick_ObstacleGen; // Set the tick function for the obstacle generation task

    tasks[2].period = TASK3_PERIOD; // Set the period for the player input task
    tasks[2].state = PI_WAIT; // Set the initial state for the player input task
    tasks[2].elapsedTime = 0; // Initialize elapsed time for the player input task
    tasks[2].TickFct = &Tick_PlayerInput; // Set the tick function for the player input task

    tasks[3].period = TASK4_PERIOD; // Set the period for the display update task
    tasks[3].state = DU_UPDATE; // Set the initial state for the display update task
    tasks[3].elapsedTime = 0; // Initialize elapsed time for the display update task
    tasks[3].TickFct = &Tick_DisplayUpdate; // Set the tick function for the display update task

    TimerSet(GCD_PERIOD); // Set the timer period to the GCD period
    TimerOn(); // Start the timer

    while (1) {
        debugIRSerial(); // Continuously check and print IR input to the serial monitor
    }

    return 0; // Return 0 (never reached)
}

// Timer interrupt service routine
void TimerISR(void) {
    for (unsigned int i = 0; i < NUM_TASKS; i++) { // Loop through all tasks
        if (tasks[i].elapsedTime >= tasks[i].period) { // If the task's period has elapsed
            tasks[i].state = tasks[i].TickFct(tasks[i].state); // Execute the task's tick function
            tasks[i].elapsedTime = 0; // Reset the task's elapsed time
        }
        tasks[i].elapsedTime += GCD_PERIOD; // Increment the task's elapsed time
    }
}
*/




#include <avr/io.h> // AVR I/O library for port and pin manipulation
#include <avr/interrupt.h> // AVR interrupt library
#include <util/delay.h> // AVR delay library for timing
#include <stdlib.h> // Standard library for random number generation
#include <stdio.h> // Standard I/O library for debugging
#include "LCD.h" // Custom library for LCD control
#include "timerISR.h" // Custom library for timer interrupt service routines
#include "periph.h" // Custom library for peripheral initialization (e.g., ADC)
#include "irAVR.h" // Include the IR remote library
#include <string.h> // String manipulation library for handling strings

// ==========================================================
//                GLOBAL VARIABLES & CONSTANTS
// ==============================================   ============
enum ObstacleStates { OBST_WAIT, OBST_GEN, OBST_SHOW };
static enum ObstacleStates obstState = OBST_WAIT;
static uint8_t obstacleLED = 0;   // 0 = Left, 1 = Middle, 2 = Right
static uint16_t obstTimer = 0;   // counts 50 ms ticks while in OBST_SHOW
static uint8_t cycleScored = 0;   // Tracks if the player has scored for this LED
static uint8_t wrongPressed = 0;  // Tracks if the player has pressed a wrong button for this LED

// -------------- Input Task Globals ---------------------
enum InputStates { INPUT_WAIT, INPUT_READ };
static enum InputStates inputState = INPUT_WAIT;
static int8_t playerInput = -1;
static decode_results irResults;

#define IR_BTN_LEFT   0x00FF22DD
#define IR_BTN_MIDDLE 0x00FF02FD
#define IR_BTN_RIGHT  0x00FFC23D
#define IR_BTN_POWER_BUTTON 0x00FFA25D
#define IR_BTN_UP    0x00FF906F
#define IR_BTN_DOWN  0x00FFE01F    

static uint8_t gameStarted = 0;
static uint8_t score = 0;
static uint8_t lives = 3;
static uint8_t buzzerFlag = 0;
static uint8_t updateLCD = 0;
static uint8_t missedInput = 0;  // Added global variable to track missed input
static uint8_t initialScreen = 1; // Flag to track the initial splash screen

// Difficulty-related globals
enum Difficulty { EASY, MEDIUM, HARD, INSANE };
static Difficulty gDifficulty = EASY;      // default
static const char* diffLabel[] = {"Easy", "Medium", "Hard", "Insane"};
static uint16_t diffLEDTime[] = {40, 30, 18, 12};  // counts of 50 ms ticks
static uint8_t diffLives[] = {3, 3, 3, 3}; // Standardize lives across all levels
static uint8_t  diffPoints[]  = {1, 1, 2, 3};

static uint16_t obstLimit = 30;  // Default LED-on time
static uint8_t pointsPerHit = 1; // Default points per hit

static uint16_t debounceTimer = 0; // Timer to debounce IR input

// New global flags and constants
#define LEVEL_UP_SCORE 10

static uint8_t levelUpPending = 0;  // 0 = no level-up, 1 = "Next Level", 2 = "Campaign Complete"
static uint16_t levelUpTimer = 0;  // Timer for "GOOD JOB!! NEXT LEVEL!" display

void resetGame() {
    // Reset core game variables
    score        = 0;
    lives        = diffLives[gDifficulty];
    playerInput  = -1;
    missedInput  = 0;
    obstState    = OBST_WAIT;
    obstTimer    = 0;
    buzzerFlag   = 0;
    updateLCD    = 1;  // force next LCD refresh to show initial screen

    // Turn off all LEDs (PD4, PD5, PD6)
    PORTD &= ~((1 << PD4) | (1 << PD5) | (1 << PD6));

    // Return to the initial “Press POWER Btn” splash screen
    initialScreen = 1;
    lcd_clear();
    lcd_write_str("Press POWER Btn");
    lcd_goto_xy(1, 0);
    lcd_write_str("to Start Game");
}

void showIdleScreen(void)
{
    /* 1) Clear the LCD display */
    lcd_clear();

    /* 2) Top row (row 0): "<   Select   >" centered */
    const char *topText = "<   Select   >";
    uint8_t topLen   = (uint8_t)strlen(topText);           /* length of topText */
    uint8_t topStart = (uint8_t)((16 - topLen) / 2);       /* starting column */

    lcd_goto_xy(0, topStart);                              /* move to row 0, col = topStart */
    lcd_write_str(topText);                                /* print "<   Select   >" */

    /* 3) Bottom row (row 1): current difficulty label centered */
    const char *botText = diffLabel[gDifficulty];
    uint8_t botLen   = (uint8_t)strlen(botText);           /* length of diffLabel string */
    uint8_t botStart = (uint8_t)((16 - botLen) / 2);       /* starting column */

    lcd_goto_xy(1, botStart);                              /* move to row 1, col = botStart */
    lcd_write_str(botText);                                /* print "Easy", "Medium", etc. */
}

void triggerGameOver() {
    gameStarted = 0;

    lcd_clear();
    lcd_write_str("Game over :((");
    _delay_ms(3000);
    lcd_clear();

    lcd_write_str("Restart Game?");
    lcd_goto_xy(1, 0);
    lcd_write_str("Up:YES! Down:No");

    while (1) {
        if (IRdecode(&irResults)) {
            if (irResults.value == IR_BTN_UP) {
                IRresume();
                score = 0;
                lives = diffLives[gDifficulty];   // Reset lives only on a true restart
                obstState = OBST_WAIT;
                obstTimer = 0;
                obstacleLED = 0xFF;
                missedInput = 0;
                updateLCD = 1;
                gameStarted = 1;
                return;
            } else if (irResults.value == IR_BTN_DOWN) {
                IRresume();
                resetGame();           // Reset game state properly
                return;                // Exit the function to follow the initial flow
            } else {
                IRresume();
            }
        }
    }
}

// ==========================================================
//                TASK: LED (Left LED = PD4, Middle LED = PD5, 
//                                 Right LED = PD6) PORTD
// ==========================================================
void TickFct_Obstacle(void) {
    if (!gameStarted) return;

    // Pause obstacle logic during "Next Level" banner
    if (levelUpPending == 1) {
        PORTD &= ~((1 << PD4) | (1 << PD5) | (1 << PD6)); // Ensure LEDs are off
        return; // Skip obstacle logic until banner finishes
    }

    /* --------- state transitions --------- */
    switch (obstState) {
        case OBST_WAIT:
            obstState = OBST_GEN;
            break;

        case OBST_GEN: {
            static uint8_t prevLED = 3;   // Initialize to an impossible value
            do {
                obstacleLED = rand() % 3;   // 0 = Left, 1 = Middle, 2 = Right
            } while (obstacleLED == prevLED);
            prevLED = obstacleLED;

            // Clear PD4‑PD6
            PORTD &= ~((1 << PD4) | (1 << PD5) | (1 << PD6));
            // Light the chosen LED
            if (obstacleLED == 0)
                PORTD |= (1 << PD4); // Left LED
            else if (obstacleLED == 1)
                PORTD |= (1 << PD5); // Middle LED
            else if (obstacleLED == 2)
                PORTD |= (1 << PD6); // Right LED

            // Reseed RNG via ADC
            ADCSRA |= (1 << ADSC);
            while (ADCSRA & (1 << ADSC));

            missedInput = 1;       // Set flag indicating input is expected
            cycleScored = 0;       // Reset scoring flag for this LED
            wrongPressed = 0;      // Reset wrong press flag for this LED
            obstState = OBST_SHOW; // Transition to OBST_SHOW
            break;
        }
        case OBST_SHOW:
            if (++obstTimer >= obstLimit) {  // Use difficulty-based LED-on time
                if (missedInput && !cycleScored) {
                    if (lives > 0) {
                        lives--;
                    }
                    buzzerFlag = 2;
                    updateLCD = 1;
                    missedInput = 0;
                }
                obstTimer = 0;
                obstState = OBST_WAIT;
            }
            break;
    }

    /* --------- state actions --------- */
    switch (obstState) {
        case OBST_SHOW:
            /* maintain LED; nothing to do */
            break;

        default:                            // OBST_WAIT
            PORTD &= ~((1 << PD4) | (1 << PD5) | (1 << PD6)); // LEDs off
            missedInput = 0;
            break;
    }
}

// ==========================================================
//                TASK: Game Start
// ==========================================================
void TickFct_GameStart(void) {
    if (!IRdecode(&irResults)) return;

    // Debounce logic: reduce debounce delay to improve responsiveness
    if (debounceTimer > 0) {
        debounceTimer--;
        IRresume();
        return;
    }

    if (initialScreen) {
        // Show the "Press POWER Btn" splash screen
        if (irResults.value == IR_BTN_POWER_BUTTON) {
            IRresume();               // Consume the POWER button press
            initialScreen = 0;        // Exit the initial screen phase
            showIdleScreen();         // Show the difficulty menu
        } else {
            IRresume();               // Ignore other inputs
        }
        return;  // Do not proceed to difficulty selection logic
    }

    // Difficulty selection logic
    if (!gameStarted) {
        switch (irResults.value) {
            case IR_BTN_LEFT:
                gDifficulty = (gDifficulty == 0) ? INSANE : (Difficulty)(gDifficulty - 1);
                showIdleScreen();
                debounceTimer = 1; // Reduce debounce timer (100ms if TickFct runs every 50ms)
                break;
            case IR_BTN_RIGHT:
                gDifficulty = (Difficulty)((gDifficulty + 1) % 4);
                showIdleScreen();
                debounceTimer = 1; // Reduce debounce timer
                break;
            case IR_BTN_MIDDLE: // Select the current difficulty
                score = 0;
                lives = diffLives[gDifficulty];   // Initialize lives only at the start
                obstLimit = diffLEDTime[gDifficulty];
                pointsPerHit = diffPoints[gDifficulty];
                gameStarted = 1;

                lcd_clear();
                lcd_write_str("Game Loading...");
                _delay_ms(1000);
                lcd_clear();
                for (int i = 3; i > 0; --i) {
                    lcd_clear();
                    char countdown[16];
                    sprintf(countdown, "Starting in...%d", i);
                    lcd_write_str(countdown);
                    _delay_ms(1000);
                }
                lcd_clear();
                lcd_write_str("Score: 0");
                lcd_goto_xy(1, 0);
                lcd_write_str("Lives: ");
                char livesStr[2];
                sprintf(livesStr, "%d", lives);
                lcd_write_str(livesStr);
                IRresume();
                break;
            default:
                IRresume();
                break;
        }
    }
}

// ==========================================================
//                INPUT TASK: IR Sensor (PD3)
// ==========================================================
void TickFct_Input(void) {
    if (!gameStarted) return;

    switch (inputState) {
        case INPUT_WAIT:
            if (IRdecode(&irResults)) {
                if (irResults.value != 0xFFFFFFFF) {
                    inputState = INPUT_READ;
                } else {
                    IRresume(); // Skip and resume IR if it's a repeat code
                }
            }
            break;

        case INPUT_READ:
            inputState = INPUT_WAIT;
            break;
    }

    switch (inputState) {
        case INPUT_READ: {
            // Compare received code to known buttons only if obstacle is visible
            if (obstState == OBST_SHOW) {
                if (irResults.value == IR_BTN_LEFT) {
                    playerInput = 0;
                }
                else if (irResults.value == IR_BTN_MIDDLE) {
                    playerInput = 1;
                }
                else if (irResults.value == IR_BTN_RIGHT) {
                    playerInput = 2;
                }
            } else {
                playerInput = -1; // Ignore input if obstacle not visible
            }

            IRresume(); // Reset IR receiver state machine
            break;
        }

        default:
            break;
    }
}

// ==========================================================
//                INPUT TASK: ScoreManagerTask
// ==========================================================
enum ScoreStates { SCORE_IDLE, SCORE_CHECK, SCORE_UPDATE };
static enum ScoreStates scoreState = SCORE_IDLE;

void TickFct_Score(void) {
    if (!gameStarted) return;

    switch (scoreState) {
        case SCORE_IDLE:
            scoreState = (playerInput != -1) ? SCORE_CHECK : SCORE_IDLE;
            break;

        case SCORE_CHECK:
            if (!cycleScored) {  // Only process input if the player hasn't scored for this LED
                if (playerInput == obstacleLED) {
                    // Correct press: award points and lock out further scoring
                    score += pointsPerHit;
                    buzzerFlag = 1;
                    cycleScored = 1;  // Mark as scored
                    missedInput = 0;  // Clear missed input flag
                    updateLCD = 1;

                    // Check if the score has reached the level-up threshold
                    if (score >= LEVEL_UP_SCORE && !levelUpPending) {
                        if (gDifficulty < INSANE) {
                            levelUpPending = 1;  // Show "Next Level" banner
                            levelUpTimer = 60;  // ~3 seconds at 50ms/tick
                        } else {
                            levelUpPending = 2;  // On Insane, go to "Campaign Complete"
                        }
                        updateLCD = 1;  // Force LCD refresh
                    }
                } else {
                    // Wrong press: deduct a life, but allow further attempts
                    if (!wrongPressed) {
                        if (lives > 0) lives--;
                        buzzerFlag = 2;
                        updateLCD = 1;
                        wrongPressed = 1;  // Mark as wrong press handled
                    }
                }
            }
            // Clear playerInput to avoid reprocessing the same button
            playerInput = -1;

            // Move to SCORE_UPDATE to return to IDLE
            scoreState = SCORE_UPDATE;
            break;

        case SCORE_UPDATE:
            scoreState = SCORE_IDLE;
            break;

        default:
            break;
    }
}

// ==========================================================
//                TIMER INTERRUPT SERVICE ROUTINE
// ==========================================================
void TimerISR(void) {
    // Placeholder for timer interrupt service routine
    // Add logic here if needed for scheduled tasks
}

// ==========================================================
//                MAIN FUNCTION
// ==========================================================
int main(void) {
    // --------------------
    // PORT INITIALIZATION
    // --------------------
    DDRD |= (1 << PD4) | (1 << PD5) | (1 << PD6); // LED pins as outputs
    PORTD &= ~((1 << PD4) | (1 << PD5) | (1 << PD6)); // Turn off LEDs

    // Configure ADC to read from an unconnected analog pin for randomness
    ADMUX = (1 << REFS0); // AVcc reference, ADC0
    ADCSRA = (1 << ADEN) | (1 << ADSC) | (1 << ADPS2); // Enable ADC, start conversion, prescaler 16
    while (ADCSRA & (1 << ADSC)); // Wait for conversion to finish
    srand(ADC); // Use analog noise for better seed

    // Show the initial splash screen
    lcd_init();
    lcd_clear();
    lcd_write_str("Press POWER Btn");
    lcd_goto_xy(1, 0);
    lcd_write_str("to Start Game");

    // IR sensor initialization (PD3)
    IRinit(&DDRD, &PIND, PD3);

    while (1) {
        TickFct_GameStart();
        TickFct_Input();
        TickFct_Score();
        TickFct_Obstacle();

        if (gameStarted && lives == 0) {
            triggerGameOver();
        }

        // —— New "level up / display" block ——
        if (initialScreen) {
            // Keep "Press POWER Btn / to Start Game" on-screen
        }
        else if (levelUpPending == 1) {
            // 1) Show "GOOD JOB!! NEXT LEVEL!" until timer hits zero
            lcd_clear();
            lcd_write_str("GOOD JOB!!");
            lcd_goto_xy(1, 0);
            lcd_write_str("NEXT LEVEL!");
            if (--levelUpTimer == 0) {
                // A) Reset obstacle state so we start fresh
                obstState = OBST_WAIT;
                obstTimer = 0;
                obstacleLED = 0xFF; // No LED lit yet
                missedInput = 0;   // Reset missed input flag

                // B) Advance difficulty & re-initialize score (but DO NOT reset lives)
                gDifficulty = (Difficulty)(gDifficulty + 1);
                score = 0;
                obstLimit = diffLEDTime[gDifficulty];
                pointsPerHit = diffPoints[gDifficulty];

                // C) Clear the banner flag; allow game to resume
                levelUpPending = 0;
                updateLCD = 1;
            }
        }
        else if (levelUpPending == 2) {
            // 2) We were on Insane → show "Campaign Complete", then Game Over prompt
            lcd_clear();
            lcd_write_str("CAMPAIGN COMPLETE");
            _delay_ms(1500);
            triggerGameOver();
            levelUpPending = 0;
        }
        else if (updateLCD && gameStarted) {
            // 3) Normal in-game display
            lcd_clear();
            char buf[16];
            sprintf(buf, "Score: %u", score);
            lcd_write_str(buf);
            lcd_goto_xy(1, 0);
            sprintf(buf, "Lives: %u", lives);
            lcd_write_str(buf);
            updateLCD = 0;
        }

        _delay_ms(50);  // 50 ms tick
    }
    return 0;
}
