#include "timerISR.h"
#include "helper.h"
#include "periph.h"



#define NUM_TASKS 2 //TODO: Change to the number of tasks being used

/* ======================= PIN MAP ======================= */
/* --- Input buttons & joystick switch (active‑LOW) --- */
#define BTN_LEFT   PC3   // A3  – left indicator button
#define BTN_RIGHT  PC4   // A4  – right indicator button
#define BTN_HORN   PC2   // A2  – joystick push‑button

/* --- Output LEDs --- */
/* Left indicator (three LEDs spread across two ports) */
#define LED_L0     PD5   // D5
#define LED_L1     PD7   // D7
#define LED_L2     PB0   // D8

/* Right indicator (contiguous on PORTD) */
#define LED_R0     PD4   // D4
#define LED_R1     PD3   // D3
#define LED_R2     PD2   // D2

/* Passive buzzer (PWM) */
#define BUZZER_PIN PD6   // D6 / OC0A

/* ============= Period constants (milliseconds) ============= */
const unsigned long IND_PERIOD  = 670;
const unsigned long HORN_PERIOD =  50;
const unsigned long GCD_PERIOD  =  10;


//Task struct for concurrent synchSMs implmentations
typedef struct _task{
	signed 	 char state; 		//Task's current state
	unsigned long period; 		//Task period
	unsigned long elapsedTime; 	//Time elapsed since last task tick
	int (*TickFct)(int); 		//Task tick function
} task;




task tasks[NUM_TASKS]; // declared task array with 5 tasks


void TimerISR() {
	for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {                   // Iterate through each task in the task array
		if ( tasks[i].elapsedTime == tasks[i].period ) {           // Check if the task is ready to tick
			tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
			tasks[i].elapsedTime = 0;                          // Reset the elapsed time for the next tick
		}
		tasks[i].elapsedTime += GCD_PERIOD;                        // Increment the elapsed time by GCD_PERIOD
	}
}


int stages[8] = {0b0001, 0b0011, 0b0010, 0b0110, 0b0100, 0b1100, 0b1000, 0b1001};//Stepper motor phases


/* --------------- Buzzer PWM initialisation (Timer0) --------------- */
static void Buzzer_InitPWM(void)
{
    DDRD  |= (1 << BUZZER_PIN);                       /* OC0A output   */
    TCCR0A = (1 << COM0A1) | (1 << WGM01) | (1 << WGM00); /* Fast PWM   */
    OCR0A  = 255;                                     /* start muted   */
}

/* --------------- Indicator state machine --------------- */
enum IND_States { IND_idle, L0, L1, L2, L3, R0, R1, R2, R3 };
int Tick_Indicator(int state)
{
    static uint8_t side = 0;               /* 0 = none, 1 = left, 2 = right */
    /* buttons are wired active‑HIGH (default low via 1 kΩ to GND) */
    uint8_t leftBtn  = (PINC & (1 << BTN_LEFT));
    uint8_t rightBtn = (PINC & (1 << BTN_RIGHT));

    /* ---- Transitions ---- */
    switch (state)
    {
        /* ---------------- Idle ---------------- */
        case IND_idle:
            if (leftBtn && !rightBtn)      { side = 1; state = L1; }
            else if (rightBtn && !leftBtn) { side = 2; state = R1; }
            break;

        /* ---------------- LEFT cumulative sweep ---------------- */
        case L1:  state = leftBtn ? L2 : IND_idle;  break;
        case L2:  state = leftBtn ? L3 : IND_idle;  break;
        case L3:  state = leftBtn ? L0 : IND_idle;  break;
        case L0:  state = leftBtn ? L1 : IND_idle;  break;

        /* ---------------- RIGHT cumulative sweep ---------------- */
        case R1:  state = rightBtn ? R2 : IND_idle;  break;
        case R2:  state = rightBtn ? R3 : IND_idle;  break;
        case R3:  state = rightBtn ? R0 : IND_idle;  break;
        case R0:  state = rightBtn ? R1 : IND_idle;  break;

        default:  state = IND_idle;                 break;
    }

    /* ---- Actions ---- */
    /* clear all indicator LEDs */
    PORTD &= ~((1<<LED_L0)|(1<<LED_L1)|(1<<LED_R0)|(1<<LED_R1)|(1<<LED_R2));
    PORTB &= ~(1<<LED_L2);

    if (side == 1) {                    /* LEFT cumulative sweep with blank */
        switch(state) {
            case L1:               PORTB |= (1<<LED_L2);                                    break; /* 1   */
            case L2:               PORTB |= (1<<LED_L2);  PORTD |= (1<<LED_L1);             break; /* 11  */
            case L3:               PORTB |= (1<<LED_L2);  PORTD |= (1<<LED_L1)|(1<<LED_L0); break; /* 111 */
            case L0:               /* all off */                                            break; /*  -  */
            default: break;
        }
    }
    else if (side == 2) {               /* RIGHT cumulative sweep with blank */
        switch(state) {
            case R1:               PORTD |= (1<<LED_R0);                                   break;
            case R2:               PORTD |= (1<<LED_R0)|(1<<LED_R1);                       break;
            case R3:               PORTD |= (1<<LED_R0)|(1<<LED_R1)|(1<<LED_R2);           break;
            case R0:               /* all off */                                            break;
            default: break;
        }
    }

    /* reset side flag if we fell back to idle */
    if (state == IND_idle)    side = 0;
    return state;
}

/* --------------- Horn (buzzer) state machine --------------- */
enum HORN_States { HORN_off, HORN_on };
int Tick_Horn(int state)
{
    /* joystick push‑button still wired active‑LOW (pull‑up) */
    uint8_t pressed = !(PINC & (1<<BTN_HORN));

    if (pressed) {
        TCCR0B = (TCCR0B & 0xF8) | 0x03;   /* prescaler 64 -> louder, clearer tone */
        OCR0A  = 128;                      /* 50% duty cycle for clean tone */
        DDRD  |= (1 << BUZZER_PIN);        /* ensure output is enabled   */
        state = HORN_on;
    } else {
        OCR0A  = 0;                        /* force LOW output            */
        TCCR0B &= 0xF8;                    /* stop clock (prescaler 0)    */
        DDRD  &= ~(1 << BUZZER_PIN);      /* set pin to input (high-Z)   */
        state = HORN_off;
    }
    return state;
}

int main(void) {
    /* -------- GPIO direction -------- */
    DDRD |= (1<<LED_L0)|(1<<LED_L1)|(1<<LED_R0)|(1<<LED_R1)|(1<<LED_R2)|(1<<BUZZER_PIN);
    DDRB |= (1<<LED_L2);
    DDRC &= ~((1<<BTN_LEFT)|(1<<BTN_RIGHT)|(1<<BTN_HORN));
    /* Enable pull‑up only for joystick button (BTN_HORN) – indicator buttons use external 1 kΩ to GND */
    PORTC  =  (1<<BTN_HORN);

    ADC_init();          /* already provided utility */
    Buzzer_InitPWM();    /* set up Timer0 for horn PWM */

    /* ---------- Task initialisation ---------- */
    tasks[0].state       = IND_idle;
    tasks[0].period      = IND_PERIOD;
    tasks[0].elapsedTime = 0;
    tasks[0].TickFct     = &Tick_Indicator;

    tasks[1].state       = HORN_off;
    tasks[1].period      = HORN_PERIOD;
    tasks[1].elapsedTime = 0;
    tasks[1].TickFct     = &Tick_Horn;

    TimerSet(GCD_PERIOD);
    TimerOn();

    while(1) { /* all work done in TimerISR */ }

    return 0;
}