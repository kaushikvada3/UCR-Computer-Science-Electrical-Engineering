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
static uint8_t marioPlayed = 0;

// Scheduler task array and periods
#define TASKS_LEN 4
Task tasks[TASKS_LEN];
const unsigned long taskPeriods[TASKS_LEN] = {
    50,   // GameStart every 50 ms
    50,   // Input every 50 ms
    50,   // Score every 50 ms
    50    // Obstacle every 50 ms
};

// Define UP and DOWN arrow characters
uint8_t upArrow[8] = {
    0b00100,
    0b01110,
    0b11111,
    0b11111,
    0b00000,
    0b00000,
    0b00000,
    0b00000
};

uint8_t downArrow[8] = {
    0b11111,
    0b11111,
    0b01110,
    0b00100,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
};

// Define sad face character
uint8_t sadFace[8] = {
    0b00000,
    0b00000,
    0b01010,
    0b00000,
    0b01110,
    0b10001,
    0b00000,
    0b00000
};

// Define custom characters for "<" and ">"
uint8_t leftArrow[8] = {
    0b00000,
    0b00000,
    0b01001,
    0b11011,
    0b11011,
    0b01001,
    0b00000,
    0b00000
};

uint8_t rightArrow[8] = {
    0b00000,
    0b00000,
    0b10010,
    0b11011,
    0b11011,
    0b10010,
    0b00000,
    0b00000
};

uint8_t arrow_line_right[8] = {
    0b00000,
    0b00011,
    0b00011,
    0b00011,
    0b00011,
    0b00011,
    0b00011,
    0b00000,
};

uint8_t arrow_line_left[8] = {
    0b00000,
    0b11000,
    0b11000,
    0b11000,
    0b11000,
    0b11000,
    0b11000,
    0b00000,
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
    lcd_clear();

    // Top row (row 0): "<   Select   >" with custom characters slightly shifted to the left
    const char *topText = " Select ";
    uint8_t topLen   = (uint8_t)strlen(topText) + 2; // Include "<" and ">"
    uint8_t topStart = (uint8_t)((16 - topLen) / 2) - 1; // Shift left by 1 column

    lcd_goto_xy(0, topStart);
    lcd_write_character(3); // Custom char for "<"
    lcd_write_character(6); // Custom char for arrow line
    lcd_write_str(topText);
    lcd_write_character(5); // Custom char for arrow line
    lcd_write_character(4); // Custom char for ">"

    // Bottom row (row 1): current difficulty label centered
    const char *botText = diffLabel[gDifficulty];
    uint8_t botLen   = (uint8_t)strlen(botText);           /* length of diffLabel string */
    uint8_t botStart = (uint8_t)((16 - botLen) / 2);       /* starting column */

    lcd_goto_xy(1, botStart);                              /* move to row 1, col = botStart */
    lcd_write_str(botText);                                /* print "Easy", "Medium", etc. */
}

// Short high‐pitch success tone at 1000 Hz
void buzzer_success(uint16_t durationMs) {
    // For 1000 Hz, half‐period = 500 us (constant)
    uint16_t cycles = durationMs; // one cycle per ms
    for (uint16_t i = 0; i < cycles; i++) {
        PORTD |= (1 << PD2);
        _delay_us(500);   // constant
        PORTD &= ~(1 << PD2);
        _delay_us(500);   // constant
    }
}

// Longer low‐pitch error tone at 500 Hz
void buzzer_error(uint16_t durationMs) {
    // For 500 Hz, period = 2000 us -> half‐period = 1000 us (constant)
    uint16_t cycles = durationMs / 2; // two ms per cycle
    for (uint16_t i = 0; i < cycles; i++) {
        PORTD |= (1 << PD2);
        _delay_us(1000); // constant
        PORTD &= ~(1 << PD2);
        _delay_us(1000); // constant
    }
    // If durationMs is odd, play one extra half‐period of 1000 us ON and 1000 us OFF
    if (durationMs % 2) {
        PORTD |= (1 << PD2);
        _delay_us(1000);
        PORTD &= ~(1 << PD2);
        _delay_us(1000);
    }
}

// Play a short Mario-like victory tune on PD2 (passive buzzer)
void playMarioTune() {
    uint16_t cycles;

    // Note frequencies (half-periods in microseconds):
    // E5: half-period ≈ 760 us, C5: half-period ≈ 956 us, G4: half-period ≈ 1517 us
    // G4 (lower octave) can be used as a bass note.

    // Play E5 for 100 ms
    cycles = (100UL * 1000UL) / (760UL * 2UL);
    for (uint16_t i = 0; i < cycles; i++) {
        PORTD |= (1 << PD2);
        _delay_us(760);
        PORTD &= ~(1 << PD2);
        _delay_us(760);
    }
    _delay_ms(50);

    // Play E5 again for 100 ms
    cycles = (100UL * 1000UL) / (760UL * 2UL);
    for (uint16_t i = 0; i < cycles; i++) {
        PORTD |= (1 << PD2);
        _delay_us(760);
        PORTD &= ~(1 << PD2);
        _delay_us(760);
    }
    _delay_ms(50);

    // Play C5 for 150 ms
    cycles = (150UL * 1000UL) / (956UL * 2UL);
    for (uint16_t i = 0; i < cycles; i++) {
        PORTD |= (1 << PD2);
        _delay_us(956);
        PORTD &= ~(1 << PD2);
        _delay_us(956);
    }
    _delay_ms(50);

    // Play G4 for 200 ms
    cycles = (200UL * 1000UL) / (1517UL * 2UL);
    for (uint16_t i = 0; i < cycles; i++) {
        PORTD |= (1 << PD2);
        _delay_us(1517);
        PORTD &= ~(1 << PD2);
        _delay_us(1517);
    }
    _delay_ms(100);
}

// Function to create custom characters using LCD.h commands
void lcd_create_char(uint8_t location, uint8_t charmap[]) {
    location &= 0x7; // Only 8 locations (0-7)
    lcd_goto_xy(0, 0); // Ensure cursor is at the start
    lcd_send_command(0x40 | (location << 3)); // Set CGRAM address
    for (int i = 0; i < 8; i++) {
        lcd_write_character(charmap[i]); // Write character data
    }
}

// Trigger the game-over sequence
void triggerGameOver() {
    gameStarted = 0;

    lcd_clear();
    lcd_write_str("Game over ");
    lcd_write_character(2); // Sad face character
    lcd_write_character(2); // Sad face character
    lcd_write_character(2); // Sad face character
    // Play “womp womp” error tone twice
    buzzer_error(150); // shorter 500 Hz error tone
    _delay_ms(100);
    buzzer_error(300); // longer 500 Hz error tone
    _delay_ms(3000);
    lcd_clear();

    lcd_write_str("Restart Game?");
    lcd_goto_xy(1, 0);
    lcd_write_character(0); // UP arrow
    lcd_write_str(":YES! ");
    lcd_write_character(1); // DOWN arrow
    lcd_write_str(":NO");

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
                // Difficulty-select beeps
                switch (gDifficulty) {
                    case EASY:
                        buzzer_success(50);
                        break;
                    case MEDIUM:
                        buzzer_success(50);
                        _delay_ms(50);
                        buzzer_success(50);
                        break;
                    case HARD:
                        buzzer_success(50);
                        _delay_ms(50);
                        buzzer_success(50);
                        _delay_ms(50);
                        buzzer_success(50);
                        break;
                    case INSANE:
                        buzzer_success(30);
                        _delay_ms(30);
                        buzzer_success(30);
                        _delay_ms(30);
                        buzzer_success(30);
                        break;
                }
                debounceTimer = 1; 
                break;
            case IR_BTN_RIGHT:
                gDifficulty = (Difficulty)((gDifficulty + 1) % 4);
                showIdleScreen();
                // Difficulty-select beeps
                switch (gDifficulty) {
                    case EASY:
                        buzzer_success(50);
                        break;
                    case MEDIUM:
                        buzzer_success(50);
                        _delay_ms(50);
                        buzzer_success(50);
                        break;
                    case HARD:
                        buzzer_success(50);
                        _delay_ms(50);
                        buzzer_success(50);
                        _delay_ms(50);
                        buzzer_success(50);
                        break;
                    case INSANE:
                        buzzer_success(30);
                        _delay_ms(30);
                        buzzer_success(30);
                        _delay_ms(30);
                        buzzer_success(30);
                        break;
                }
                debounceTimer = 1;
                break;
            case IR_BTN_MIDDLE:
                score = 0;
                lives = diffLives[gDifficulty];
                obstLimit   = diffLEDTime[gDifficulty];
                pointsPerHit = diffPoints[gDifficulty];
                gameStarted = 1;

                // Play a start tone
                buzzer_success(200); // 1 kHz tone for 200 ms

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
                            levelUpTimer = 36;
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
                        missedInput = 0;  // Prevent double penalty
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

    DDRD |= (1 << PD2);  // Configure PD2 as output for the buzzer

    // ADC for randomness
    ADMUX  = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADSC) | (1 << ADPS2);
    while (ADCSRA & (1 << ADSC));
    srand(ADC);

    // Initialize LCD and load custom characters
    lcd_init();
    lcd_clear();
    lcd_create_char(0, upArrow);    // Custom char 0 = UP arrow
    lcd_create_char(1, downArrow);  // Custom char 1 = DOWN arrow
    lcd_create_char(2, sadFace);    // Custom char 2 = Sad face
    lcd_create_char(3, leftArrow);  // Custom char 3 = "<"
    lcd_create_char(4, rightArrow); // Custom char 4 = ">"
    lcd_create_char(5, arrow_line_right); // Custom char 5 = arrow line
    lcd_create_char(6, arrow_line_left);

    // Test slide-in animation on the top row
    lcd_display_slide_animation("Welcome Player!", 0, 100);
    lcd_display_slide_animation("Lets GO!!", 1, 100);

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
            // lcd_goto_xy(1, 0);
            // lcd_write_character(2); // Display sad face (removed as per requirement)
            if (!marioPlayed) {
                playMarioTune();
                marioPlayed = 1;
            }
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
                marioPlayed = 0;
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
            buzzer_success(80);  // Success tone
            buzzerFlag = 0;
        }
        else if (buzzerFlag == 2) {
            for (uint8_t i = 0; i < 3; i++) {
                buzzer_error(60); // Error tone
                _delay_ms(60);
            }
            buzzerFlag = 0;
        }

        _delay_ms(50);
    }
    return 0;
}
