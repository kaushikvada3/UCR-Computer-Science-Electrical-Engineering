#include <stdint.h>

// Actual implementation for lcd_write_cmd and lcd_write_data
#include <avr/io.h>
#include <util/delay.h>
void lcd_write_cmd(uint8_t cmd) {
    PORTC &= ~(1 << PC0); // RS = 0 for command
    PORTC &= ~(1 << PC1); // RW = 0 for write
    PORTD = cmd;
    PORTC |= (1 << PC2);  // E = 1
    _delay_us(1);
    PORTC &= ~(1 << PC2); // E = 0
    _delay_ms(2);
}

void lcd_write_data(uint8_t data) {
    PORTC |= (1 << PC0);  // RS = 1 for data
    PORTC &= ~(1 << PC1); // RW = 0 for write
    PORTD = data;
    PORTC |= (1 << PC2);  // E = 1
    _delay_us(1);
    PORTC &= ~(1 << PC2); // E = 0
    _delay_ms(2);
}

// ==========================================================
//                LCD CUSTOM CHARACTER FUNCTIONS
// ==========================================================
void lcd_create_char(uint8_t location, uint8_t charmap[]) {
    location &= 0x7; // Only 0-7 allowed
    lcd_write_cmd(0x40 | (location << 3));
    for (int i = 0; i < 8; i++) {
        lcd_write_data(charmap[i]);
    }
}

void lcd_write_char(uint8_t location) {
    lcd_write_data(location);
}
#include <avr/io.h> // AVR I/O library for port and pin manipulation
#include <avr/interrupt.h> // AVR interrupt library
#include <util/delay.h> // AVR delay library for timing
#include <stdlib.h> // Standard library for random number generation
#include <stdio.h> // Standard I/O library for debugging
#include "LCD.h" // Custom library for LCD control
#include "timerISR.h" // Custom library for timer interrupt service routines
#include "periph.h" // Custom library for peripheral initialization (e.g., ADC)
#include "irAVR.h" // Include the IR remote library

int uart_putchar(char c, FILE *stream) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
    return 0;
}

// ==========================================================
//                GLOBAL VARIABLES & CONSTANTS
// ==========================================================
enum ObstacleStates { OBST_WAIT, OBST_GEN, OBST_SHOW };
static enum ObstacleStates obstState = OBST_WAIT;
static uint8_t obstacleLED = 0;   // 0 = Left, 1 = Middle, 2 = Right
static uint16_t obstTimer = 0;   // counts 50 ms ticks while in OBST_SHOW

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

void resetGame() {
    score        = 0;
    lives        = 3;
    playerInput  = -1;
    missedInput  = 0;
    obstState    = OBST_WAIT;
    obstTimer    = 0;
    buzzerFlag   = 0;
    updateLCD    = 1;       // force HUD refresh
    PORTD &= ~((1 << PD4) | (1 << PD5) | (1 << PD6));   // LEDs off
}

void triggerGameOver();

// ==========================================================
//                TASK: LED (Left LED = PD4, Middle LED = PD5, 
//                                 Right LED = PD6) PORTD
// ==========================================================
void TickFct_Obstacle(void) {
    if (!gameStarted) return;
    /* --------- state transitions --------- */
    switch (obstState) {
        case OBST_WAIT:
            obstState = OBST_GEN;
            break;

        case OBST_GEN:
            obstState = OBST_SHOW;
            break;

        case OBST_SHOW:
            if (++obstTimer >= 30.783) {        // 30.783 × 50 ms = 1.539 s
                if (missedInput && playerInput == -1) {
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
    static uint8_t prevLED = 3;   // initialise to impossible value
    switch (obstState) {
        case OBST_GEN: {
            do {
                obstacleLED = rand() % 3;   // 0,1,2
            } while (obstacleLED == prevLED);
            prevLED = obstacleLED;
            // clear PD4‑PD6
            PORTD &= ~((1 << PD4) | (1 << PD5) | (1 << PD6));
            // light the chosen LED
            if (obstacleLED == 0)
                PORTD |= (1 << PD4); // Left LED
            else if (obstacleLED == 1)
                PORTD |= (1 << PD5); // Middle LED
            else // obstacleLED == 2
                PORTD |= (1 << PD6); // Right LED
            // reseed rand each time for more unpredictability
            ADCSRA |= (1 << ADSC); // start new ADC conversion
            while (ADCSRA & (1 << ADSC)); // wait for conversion
            // srand(ADC); // moved to main()
            // char buffer[20];
            // sprintf(buffer, "LED: %u", obstacleLED);
            // lcd_clear();
            // lcd_write_str(buffer);
            missedInput = 1;  // Added: set flag indicating input expected
            break;
        }
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
    if (!gameStarted && IRdecode(&irResults)) {
        if (irResults.value == IR_BTN_POWER_BUTTON) {
            resetGame();
            gameStarted = 1;
            lcd_clear();
            lcd_write_str("Game Loading...");
            _delay_ms(1000);
            lcd_clear();
            lcd_write_str("Starting in...");
            _delay_ms(1000);
            for (int i = 3; i > 0; --i) {
                lcd_clear();
                lcd_write_str("Starting in...");
                lcd_goto_xy(1, 0);
                char countdown[2];
                sprintf(countdown, "%d", i);
                lcd_write_str(countdown);
                _delay_ms(1000);
            }
            lcd_clear();
            lcd_write_str("Score: 0");
            lcd_goto_xy(1, 0);
            lcd_write_str("Lives: 3");
            IRresume();
        } else {
            IRresume();
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
            // Constantly poll the IR decoding function
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
            // Print the raw IR code in hex to the serial monitor
            char hexBuffer[20];
            sprintf(hexBuffer, "IR Code: %08lX\n", irResults.value);
            printf("%s", hexBuffer);

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

            // Reset IR receiver state machine
            IRresume();
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
            scoreState = SCORE_UPDATE;
            break;

        case SCORE_UPDATE:
            scoreState = SCORE_IDLE;
            if (lives > 0) {
                playerInput = -1;
            }
            break;
    }

    /* ---------- state actions ---------- */
    switch (scoreState) {
        case SCORE_CHECK:
            if (playerInput == obstacleLED) {
                score++;
                buzzerFlag = 1;
            } else {
                if (lives > 0) lives--;
                buzzerFlag = 2;
            }
            missedInput = 0;          // ✔ clear: we already reacted to this turn
            updateLCD   = 1;
            break;

        default:
            break;
    }
}   // <-- ends TickFct_Score

// ==========================================================
//                GAME OVER HANDLER
// ==========================================================
void triggerGameOver() {
    gameStarted = 0;

    lcd_clear();
    lcd_write_str("Game over :((");
    _delay_ms(3000);
    lcd_clear();

    uint8_t upArrow[8] = {
        0b00100, 0b01110, 0b11111, 0b00100,
        0b00100, 0b00100, 0b00100, 0b00100
    };
    uint8_t downArrow[8] = {
        0b00100, 0b00100, 0b00100, 0b00100,
        0b00100, 0b11111, 0b01110, 0b00100
    };
    lcd_create_char(0, upArrow);
    lcd_create_char(1, downArrow);

    lcd_write_str("Restart Game?");
    lcd_goto_xy(1, 0);
    lcd_write_char(0); lcd_write_str("Up:YES! ");
    lcd_write_char(1); lcd_write_str("Down:No");

    while (1) {
        if (IRdecode(&irResults)) {
            if (irResults.value == IR_BTN_UP) {
                IRresume();
                resetGame();
                gameStarted = 1;
                break;
            } else if (irResults.value == IR_BTN_DOWN) {
                IRresume();
                while (1) ;   // Halt forever
            } else {
                IRresume();
            }
        }
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

    // Initialize LCD
    lcd_init();
    lcd_clear();
    
    lcd_write_str("Press POWER Btn");
    lcd_goto_xy(1, 0);
    lcd_write_str("to Start Game");

    // IR sensor initialization (PD3)
    IRinit(&DDRD, &PIND, PD3);

    // Initialize UART for serial output
    UBRR0H = 0;
    UBRR0L = 8; // 115200 baud rate for 16MHz clock
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
    fdevopen(&uart_putchar, 0); // Enable printf to work via UART

    while (1) {
        TickFct_GameStart();
        TickFct_Input();      // Run input FSM first to update playerInput
        TickFct_Score();      // Score task evaluates updated input
        TickFct_Obstacle();   // Obstacle generation remains
        if (gameStarted && lives == 0) {
            triggerGameOver();
        }

        if (updateLCD) {
            lcd_clear();
            char buf[16];
            sprintf(buf, "Score: %u", score);
            lcd_write_str(buf);
            lcd_goto_xy(1,0);
            sprintf(buf, "Lives: %u", lives);
            lcd_write_str(buf);
            updateLCD = 0;
        }

        _delay_ms(50);        // 50 ms software tick
    }
    return 0;
}























// ==========================================================
//                      INCLUDE HEADERS
// ==========================================================
#include <avr/io.h>         // AVR I/O library for port and pin manipulation
#include <avr/interrupt.h>  // AVR interrupt library
#include <util/delay.h>     // AVR delay library for timing
#include <stdlib.h>         // Standard library for random number generation
#include <stdio.h>          // Standard I/O library for debugging
#include "LCD.h"            // Custom library for LCD control
#include "timerISR.h"       // Custom library for timer interrupt service routines
#include "periph.h"         // Custom library for peripheral initialization (e.g., ADC)
#include "irAVR.h"          // Include the IR remote library
#include <string.h>         // String manipulation library for handling strings

// ==========================================================
//                GLOBAL VARIABLES & CONSTANTS
// ==========================================================

// Task structure definition
typedef struct {
    int state;                  // Current state
    unsigned long period;       // Tick period (in ms)
    unsigned long elapsedTime;  // Time since last tick
    int (*TickFct)(int);        // Pointer to state machine function
} Task;

// Enumerations for states and difficulty levels
enum ObstacleStates { OBST_WAIT, OBST_GEN, OBST_SHOW };
enum InputStates    { INPUT_WAIT, INPUT_READ };
enum ScoreStates    { SCORE_IDLE, SCORE_CHECK, SCORE_UPDATE };
enum Difficulty     { EASY, MEDIUM, HARD, INSANE };

// Obstacle task globals
static enum ObstacleStates obstState = OBST_WAIT;
static uint8_t obstacleLED = 0;      // 0 = Left, 1 = Middle, 2 = Right
static uint16_t obstTimer   = 0;     // Counts 50 ms ticks while in OBST_SHOW
static uint8_t cycleScored  = 0;     // Tracks if the player has scored for this LED
static uint8_t wrongPressed = 0;     // Tracks if the player has pressed a wrong button for this LED

// Input task globals
static int8_t playerInput  = -1;
static decode_results irResults;

// IR button definitions
#define IR_BTN_LEFT   0x00FF22DD
#define IR_BTN_MIDDLE 0x00FF02FD
#define IR_BTN_RIGHT  0x00FFC23D
#define IR_BTN_POWER_BUTTON 0x00FFA25D
#define IR_BTN_UP      0x00FF906F
#define IR_BTN_DOWN    0x00FFE01F    

// Game state variables
static uint8_t gameStarted   = 0;
static uint8_t score         = 0;
static uint8_t lives         = 3;
static uint8_t buzzerFlag    = 0;
static uint8_t updateLCD     = 0;
static uint8_t missedInput   = 0;    // Track missed input
static uint8_t initialScreen = 1;    // Flag for initial splash screen

// Difficulty-related globals
static Difficulty gDifficulty      = EASY;    // Default start at Easy
static const char* diffLabel[]     = {"Easy", "Medium", "Hard", "Insane"};
static uint16_t diffLEDTime[]      = {26, 19, 15, 10};  // Converted to integers
static uint8_t diffLives[]         = {3, 3, 3, 3};      // Lives for each difficulty
static uint8_t diffPoints[]        = {1, 1, 1, 1};      // Points per correct hit

static uint16_t obstLimit    = 30;  // Default LED-on time (50 ms ticks)
static uint8_t pointsPerHit  = 1;   // Default points per correct hit

// Debounce timer
static uint16_t debounceTimer = 0;  // Debounce timer in ticks

// Level-up constants
#define LEVEL_UP_SCORE 10
static uint8_t levelUpPending = 0;  // 0 = no level-up, 1 = show Next Level, 2 = Campaign Complete
static uint16_t levelUpTimer  = 0;  // Counts ticks for level-up banner (~3 seconds)

// Scheduler task array and periods
#define TASKS_LEN 4
Task tasks[TASKS_LEN];
const unsigned long taskPeriods[TASKS_LEN] = {
    50,   // GameStart every 50 ms
    50,   // Input every 50 ms
    50,   // Score every 50 ms
    50    // Obstacle every 50 ms
};

// ==========================================================
//                      HELPER FUNCTIONS
// ==========================================================

// Reset the game to its initial state
void resetGame() {
    score       = 0;
    lives       = diffLives[gDifficulty];
    playerInput = -1;
    missedInput = 0;
    buzzerFlag  = 0;
    updateLCD   = 1;
    gameStarted = 0;

    // Reset obstacle flags and turn off LEDs
    obstTimer    = 0;
    cycleScored  = 0;
    wrongPressed = 0;
    PORTD &= ~((1 << PD4) | (1 << PD5) | (1 << PD6));

    // Return to initial screen
    initialScreen = 1;
    lcd_clear();
    lcd_write_str("Press POWER Btn");
    lcd_goto_xy(1, 0);
    lcd_write_str("to Start Game ->");
}

// Display the idle screen with difficulty selection
void showIdleScreen(void) {
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

// Trigger the game-over sequence
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
                // Restart at same difficulty (keep gDifficulty)
                score         = 0;
                lives         = diffLives[gDifficulty];
                cycleScored   = 0;
                wrongPressed  = 0;
                missedInput   = 0;
                gameStarted   = 1;
                return;
            }
            else if (irResults.value == IR_BTN_DOWN) {
                IRresume();
                resetGame();
                return;
            }
            else {
                IRresume();
            }
        }
    }
}

// Initialize the buzzer (PWM on PD2)
void buzzerInit() {
    DDRD |= (1 << PD2); // Set PD2 as output
    TCCR2A = (1 << WGM21) | (1 << COM2B0); // CTC mode, toggle OC2B on compare match
    TCCR2B = (1 << CS21); // Prescaler = 8
}

// Play a tone on the buzzer
void playTone(uint16_t frequency, uint16_t durationMs) {
    uint16_t ocrValue = (F_CPU / (2 * 8 * frequency)) - 1; // Calculate OCR2A value for the desired frequency
    OCR2A = ocrValue; // Set the frequency
    TCNT2 = 0; // Reset timer counter

    uint16_t elapsedTime = 0;
    while (elapsedTime < durationMs) {
        _delay_ms(1);
        elapsedTime++;
    }

    TCCR2A &= ~(1 << COM2B0); // Stop toggling OC2B
}

// ==========================================================
//                  TASK: Obstacle State Machine
// ==========================================================
int TickFct_Obstacle(int state) {
    if (!gameStarted || levelUpPending == 1) {
        // If game not started, or during level-up banner, turn off LEDs
        PORTD &= ~((1 << PD4) | (1 << PD5) | (1 << PD6));
        return state;
    }

    switch (state) {
        case OBST_WAIT:
            state = OBST_GEN;
            break;

        case OBST_GEN: {
            static uint8_t prevLED = 3;
            do {
                obstacleLED = rand() % 3;
            } while (obstacleLED == prevLED);
            prevLED = obstacleLED;

            // Turn off all LEDs first
            PORTD &= ~((1 << PD4) | (1 << PD5) | (1 << PD6));
            // Turn on the chosen LED
            if (obstacleLED == 0) PORTD |= (1 << PD4);
            else if (obstacleLED == 1) PORTD |= (1 << PD5);
            else                 PORTD |= (1 << PD6);

            // Reseed RNG using ADC noise
            ADCSRA |= (1 << ADSC);
            while (ADCSRA & (1 << ADSC));

            missedInput  = 1;    // We expect input now
            cycleScored  = 0;    // Reset scoring for this LED
            wrongPressed = 0;    // Reset wrong-press flag
            obstTimer    = 0;
            state = OBST_SHOW;
            break;
        }

        case OBST_SHOW:
            if (++obstTimer >= obstLimit) {
                if (missedInput && !cycleScored) {
                    if (lives > 0) lives--;
                    buzzerFlag = 2;
                    updateLCD = 1;
                    missedInput = 0;
                }
                obstTimer = 0;
                state = OBST_WAIT;
            }
            break;
    }

    switch (state) {
        case OBST_WAIT:
            PORTD &= ~((1 << PD4) | (1 << PD5) | (1 << PD6));
            break;
        case OBST_SHOW:
        case OBST_GEN:
            // leave the LED on
            break;
    }

    return state;
}

// ==========================================================
//                  TASK: GameStart State Machine
// ==========================================================
int TickFct_GameStart(int state) {
    if (!IRdecode(&irResults)) return state;

    // Debounce
    if (debounceTimer > 0) {
        debounceTimer--;
        IRresume();
        return state;
    }

    // Declare variables outside the switch block
    uint16_t totalDelay = 2500; // Total delay for the progress bar
    uint16_t elapsedTime = 0;

    if (initialScreen) {
        if (irResults.value == IR_BTN_POWER_BUTTON) {
            IRresume();
            initialScreen = 0;
            showIdleScreen();
        } else {
            IRresume();
        }
        return state;
    }

    if (!gameStarted) {
        switch (irResults.value) {
            case IR_BTN_LEFT:
                gDifficulty = (gDifficulty == 0) ? INSANE : (Difficulty)(gDifficulty - 1);
                showIdleScreen();
                debounceTimer = 1; 
                break;
            case IR_BTN_RIGHT:
                gDifficulty = (Difficulty)((gDifficulty + 1) % 4);
                showIdleScreen();
                debounceTimer = 1; 
                break;
            case IR_BTN_MIDDLE:
                score = 0;
                lives = diffLives[gDifficulty];
                obstLimit   = diffLEDTime[gDifficulty];
                pointsPerHit = diffPoints[gDifficulty];
                gameStarted = 1;

                // Play a start tone
                playTone(1000, 200); // 1 kHz tone for 200 ms

                // Show "Game Loading..." with a randomized progress bar
                lcd_clear();
                lcd_write_str("Game Loading...");
                lcd_goto_xy(1, 0);

                for (int i = 0; i < 16; ++i) {
                    uint16_t delay = rand() % 200 + 50; // Random delay between 50 and 250 ms
                    if (elapsedTime + delay > totalDelay) {
                        delay = totalDelay - elapsedTime; // Adjust to fit within totalDelay
                    }
                    lcd_write_character(0xFF); // 0xFF is the filled block character

                    // Use a loop to handle dynamic delays
                    while (delay > 0) {
                        _delay_ms(1);
                        delay--;
                    }
                    elapsedTime += delay;
                }

                // Countdown before starting the game
                lcd_clear();
                for (int i = 3; i > 0; --i) {
                    lcd_clear();
                    char countdown[16];
                    sprintf(countdown, "Starting in...%d", i);
                    lcd_write_str(countdown);
                    _delay_ms(1000);
                }

                // Initialize game screen
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
    return state;
}

// ==========================================================
//                    TASK: Input State Machine
// ==========================================================
int TickFct_Input(int state) {
    if (!gameStarted) return state;

    switch (state) {
        case INPUT_WAIT:
            if (IRdecode(&irResults)) {
                if (irResults.value != 0xFFFFFFFF) {
                    state = INPUT_READ;
                } else {
                    IRresume();
                }
            }
            break;
        case INPUT_READ:
            state = INPUT_WAIT;
            break;
    }

    if (state == INPUT_READ) {
        if (obstacleLED == 0 || obstacleLED == 1 || obstacleLED == 2) {
            if      (irResults.value == IR_BTN_LEFT)   playerInput = 0;
            else if (irResults.value == IR_BTN_MIDDLE) playerInput = 1;
            else if (irResults.value == IR_BTN_RIGHT)  playerInput = 2;
            else                                       playerInput = -1;
        } else {
            playerInput = -1;
        }
        IRresume();
    }

    return state;
}

// ==========================================================
//                  TASK: ScoreManager State Machine
// ==========================================================
int TickFct_Score(int state) {
    if (!gameStarted) return state;

    switch (state) {
        case SCORE_IDLE:
            state = (playerInput != -1) ? SCORE_CHECK : SCORE_IDLE;
            break;

        case SCORE_CHECK:
            if (!cycleScored) {
                if (playerInput == obstacleLED) {
                    score += pointsPerHit;
                    buzzerFlag = 1;
                    cycleScored = 1;
                    missedInput = 0;
                    updateLCD = 1;
                    if (score >= LEVEL_UP_SCORE && !levelUpPending) {
                        if (gDifficulty < INSANE) {
                            levelUpPending = 1;
                            levelUpTimer = 60;
                        } else {
                            levelUpPending = 2;
                        }
                        updateLCD = 1;
                    }
                } else {
                    if (!wrongPressed) {
                        if (lives > 0) lives--;
                        buzzerFlag = 2;
                        updateLCD = 1;
                        wrongPressed = 1;
                    }
                }
            }
            playerInput = -1;
            state = SCORE_UPDATE;
            break;

        case SCORE_UPDATE:
            state = SCORE_IDLE;
            break;
    }
    return state;
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
    // PORT INITIALIZATION
    DDRD |= (1 << PD4) | (1 << PD5) | (1 << PD6); 
    PORTD &= ~((1 << PD4) | (1 << PD5) | (1 << PD6));

    // Initialize buzzer
    buzzerInit();

    // ADC for randomness
    ADMUX  = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADSC) | (1 << ADPS2);
    while (ADCSRA & (1 << ADSC));
    srand(ADC);

    // Initialize LCD and show splash
    lcd_init();
    lcd_clear();

    // Test slide-in animation
    lcd_display_slide_animation("Welcome Player!", 1, 100);

    // Show the next message after the animation
    lcd_clear();
    lcd_write_str("Press POWER Btn");
    lcd_goto_xy(1, 0);
    lcd_write_str("to Start Game ->");

    // Initialize IR
    IRinit(&DDRD, &PIND, PD3);

    // Initialize the task array
    // Task 0: GameStart
    tasks[0].state       = INPUT_WAIT; 
    tasks[0].period      = taskPeriods[0];
    tasks[0].elapsedTime = tasks[0].period;
    tasks[0].TickFct     = &TickFct_GameStart;
    // Task 1: Input
    tasks[1].state       = INPUT_WAIT;
    tasks[1].period      = taskPeriods[1];
    tasks[1].elapsedTime = tasks[1].period;
    tasks[1].TickFct     = &TickFct_Input;
    // Task 2: Score
    tasks[2].state       = SCORE_IDLE;
    tasks[2].period      = taskPeriods[2];
    tasks[2].elapsedTime = tasks[2].period;
    tasks[2].TickFct     = &TickFct_Score;
    // Task 3: Obstacle
    tasks[3].state       = OBST_WAIT;
    tasks[3].period      = taskPeriods[3];
    tasks[3].elapsedTime = tasks[3].period;
    tasks[3].TickFct     = &TickFct_Obstacle;

    while (1) {
        // Dispatch tasks in round-robin, each 50 ms apart
        for (uint8_t i = 0; i < TASKS_LEN; i++) {
            if (tasks[i].elapsedTime >= tasks[i].period) {
                tasks[i].state = tasks[i].TickFct(tasks[i].state);
                tasks[i].elapsedTime = 0;
            }
            tasks[i].elapsedTime += 50;
        }

        // Check game-over
        if (gameStarted && lives == 0) {
            triggerGameOver(); // Show "Game Over :((" screen only when lives reach zero
        }

        // Level-up / Campaign logic
        if (initialScreen) {
            // Do nothing; splash stays on
        }
        else if (levelUpPending == 1) {
            lcd_clear();
            lcd_write_str("GOOD JOB!!");
            lcd_goto_xy(1, 0);
            lcd_write_str("NEXT LEVEL!");
            if (--levelUpTimer == 0) {
                obstState = OBST_WAIT;
                obstTimer = 0;
                obstacleLED = 0xFF;
                missedInput = 0;
                gDifficulty = (Difficulty)(gDifficulty + 1);
                score = 0;
                obstLimit   = diffLEDTime[gDifficulty];
                pointsPerHit= diffPoints[gDifficulty];
                levelUpPending = 0;
                updateLCD = 1;
            }
        }
        else if (levelUpPending == 2) {
            lcd_clear();
            lcd_write_str("GAME COMPLETE");
            _delay_ms(1500);
            resetGame(); // Reset the game instead of showing "Game Over :(("
            levelUpPending = 0;
        }
        else if (updateLCD && gameStarted) {
            lcd_clear();
            char buf[16];
            sprintf(buf, "Score: %u", score);
            lcd_write_str(buf);
            lcd_goto_xy(1, 0);
            sprintf(buf, "Lives: %u", lives);
            lcd_write_str(buf);
            updateLCD = 0;
        }

        // Buzzer logic
        if (buzzerFlag == 1) {
            playTone(1000, 80); // Play a 1 kHz tone for 80 ms
            buzzerFlag = 0;
        }
        else if (buzzerFlag == 2) {
            for (uint8_t i = 0; i < 3; i++) {
                playTone(500, 60); // Play a 500 Hz tone for 60 ms
                _delay_ms(60);
            }
            buzzerFlag = 0;
        }

        _delay_ms(50);
    }
    return 0;
}
