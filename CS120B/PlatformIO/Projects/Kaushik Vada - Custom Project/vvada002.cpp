#include <avr/io.h> // AVR I/O library for port and pin manipulation
#include <avr/interrupt.h> // AVR interrupt library
#include <util/delay.h> // AVR delay library for timing
#include <stdlib.h> // Standard library for random number generation
#include <stdio.h> // Standard I/O library for debugging
#include "LCD.h" // Custom library for LCD control
#include "timerISR.h" // Custom library for timer interrupt service routines
#include "periph.h" // Custom library for peripheral initialization (e.g., ADC)
#include "irAVR.h" // Include the IR remote library

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

    lcd_write_str("Restart Game?");
    lcd_goto_xy(1, 0);
    lcd_write_str("Up:YES! Down:No");

    while (1) {
        if (IRdecode(&irResults)) {
            if (irResults.value == IR_BTN_UP) {
                IRresume();
                resetGame();
                gameStarted = 1;
                break;
            } else if (irResults.value == IR_BTN_DOWN) {
                IRresume();
                while (1);   // Halt forever
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
