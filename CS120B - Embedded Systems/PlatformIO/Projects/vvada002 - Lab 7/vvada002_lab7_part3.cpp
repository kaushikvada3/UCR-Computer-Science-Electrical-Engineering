#include "timerISR.h"
#include "helper.h"
#include "periph.h"



#define NUM_TASKS 5 //TODO: Change to the number of tasks being used

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

/* --- Joystick analog vertical axis (gas/brake) --- */
#define JOY_Y_CH     0      // ADC0  (A0/PC0)

/* --- Joystick analog horizontal axis (steering) --- */
#define JOY_X_CH     1      // ADC1  (A1/PC1) for steering
#define SERVO_PIN    PB1    // D9 → Servo signal pin (OC1A)
/* --------------- Servo PWM initialisation (Timer1) --------------- */
static void Servo_InitPWM(void)
{
    DDRB |= (1 << SERVO_PIN);                  // Set D9 as output
    TCCR1A = (1<<COM1A1) | (1<<WGM11);          // Non-inverting, mode 14 (fast PWM)
    TCCR1B = (1<<WGM13) | (1<<WGM12) | (1<<CS11); // Prescaler 8
    ICR1 = 39999;                               // TOP for 50Hz (20ms) with 16MHz clock and /8 prescaler
}

/* --------------- Servo steering task --------------- */
int Tick_Servo(int state)
{
    uint16_t adc = ADC_read(JOY_X_CH);    // 0–1023 from left to right
    int32_t pulse = 3000;                 // Default: center (1.5ms)

    // Deadband for joystick center
    const int16_t DEADBAND = 50;

    if (adc < (512 - DEADBAND)) {
        // Joystick LEFT: sweep to +90° (2.5ms pulse)
        pulse = 5000;
    } else if (adc > (512 + DEADBAND)) {
        // Joystick RIGHT: sweep to -90° (0.5ms pulse)
        pulse = 1000;
    }
    // else: stay at center

    OCR1A = (uint16_t)pulse;
    return 0;
}

/* --- Stepper motor coil pins (4‑wire half‑step) --- */
#define STEP_A       PB5    // D13 → IN1
#define STEP_B       PB4    // D12 → IN2
#define STEP_C       PB3    // D11 → IN3
#define STEP_D       PB2    // D10 → IN4

/* ============= Period constants (milliseconds) ============= */
const unsigned long IND_PERIOD  = 670;
const unsigned long HORN_PERIOD =  50;
const unsigned long GCD_PERIOD  =  1;


//Task struct for concurrent synchSMs implmentations
typedef struct _task{
	signed 	 char state; 		//Task's current staten
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

/* ---------- Stepper‑motor control ---------- */
static const uint8_t STEP_SEQ[8] = {
    (1<<PB2),
    (1<<PB2)|(1<<PB3),
    (1<<PB3),
    (1<<PB3)|(1<<PB4),
    (1<<PB4),
    (1<<PB4)|(1<<PB5),
    (1<<PB5),
    (1<<PB5)|(1<<PB2)
};

#define SPEED_MIN_MS   30    /* slowest: 5 ms per phase  */
#define SPEED_MAX_MS   2     /* fastest: 0 ms per phase (maximum speed) */
#define DEADBAND        50   /* ADC counts around centre  */
volatile uint8_t brakeActive = 0;  /* asserted when joystick pulled down */
static uint8_t stepIdx = 0;
static uint16_t stepElapsed = 0;   /* ms accumulated since last phase */
/* --------------- Stepper motor state machine --------------- */
enum STEP_States { STEP_idle, STEP_run };
int Tick_Stepper(int state)
{
    uint16_t adc = ADC_read(JOY_Y_CH);          /* 0‑1023 */
    int16_t  delta = (int16_t)adc - 512;        /* joystick up = negative delta, down = positive delta */
    uint16_t absDelta = (delta >= 0) ? delta : -delta;

    /* Determine requested interval (ms per phase) */
    uint16_t interval = SPEED_MIN_MS;           /* default slowest     */
    if (absDelta > DEADBAND) {
        uint32_t scale = ((uint32_t)(absDelta - DEADBAND)) * (SPEED_MIN_MS - SPEED_MAX_MS);
        interval = SPEED_MIN_MS - (scale / (512 - DEADBAND));
        if (interval < SPEED_MAX_MS) interval = SPEED_MAX_MS;
    }

    /* ------------ Transitions ------------ */
    switch(state) {
        case STEP_idle:
            if (absDelta > DEADBAND) { state = STEP_run; }
            break;
        case STEP_run:
            if (absDelta <= DEADBAND) { state = STEP_idle; }
            break;
        default: state = STEP_idle; break;
    }

    /* ------------ Actions ---------------- */
    if (state == STEP_idle) {
        PORTB &= ~((1<<STEP_A)|(1<<STEP_B)|(1<<STEP_C)|(1<<STEP_D)); /* de‑energise */
        brakeActive = 0;
        stepElapsed = 0;
    } else {
        /* direction & stepping */
        stepElapsed += GCD_PERIOD;
        if (stepElapsed >= interval) {
            stepElapsed = 0;
            if (delta < -DEADBAND) {            // joystick up = drive (clockwise)
                stepIdx = (stepIdx + 1) & 0x07; // increment for up
                brakeActive = 1;                // NO beep when driving
            } else if (delta > DEADBAND) {      // joystick down = reverse (counterclockwise)
                stepIdx = (stepIdx + 7) & 0x07; // decrement for down
                brakeActive = 0;                // Beep when reversing
            } else {
                brakeActive = 1;
            }
            PORTB = (PORTB & 0x03) | STEP_SEQ[stepIdx];
        }
    }
    return state;
}

/* --------------- Brake beeper (distinct tone) --------------- */
enum BEEP_States { BEEP_idle, BEEP_on, BEEP_off };
int Tick_Beep(int state)
{
    static uint16_t timer = 0;          /* ms counter within 2 s frame */
    uint8_t hornPressed = !(PINC & (1<<BTN_HORN));

    /* --------- Transitions ---------- */
    switch(state) {
        case BEEP_idle:
            if (brakeActive && !hornPressed) { timer = 0; state = BEEP_on; }
            break;
        case BEEP_on:
            if (!brakeActive || hornPressed) { state = BEEP_idle; }
            else if (timer >= 1000)          { timer = 0; state = BEEP_off; }
            break;
        case BEEP_off:
            if (!brakeActive || hornPressed) { state = BEEP_idle; }
            else if (timer >= 1000)          { timer = 0; state = BEEP_on; }
            break;
        default: state = BEEP_idle; break;
    }

    /* --------- Actions --------------- */
    if (state == BEEP_on) {
        /* 500 Hz tone, prescaler 64, 50% duty (distinct from horn) */
        TCCR0B = (TCCR0B & 0xF8) | 0x03;
        OCR0A  = 249;               /* 16 MHz/(2*64*500 Hz) - 1 ≈ 249 for reverse tone */
        DDRD  |= (1<<BUZZER_PIN);
    } else { /* idle or off portion */
        DDRD  &= ~(1<<BUZZER_PIN);
        OCR0A  = 0;
        TCCR0B &= 0xF8;             /* stop clock */
    }

    timer += 10; // period of Tick_Beep is 10 ms
    return state;
}

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
    DDRB |= (1<<LED_L2)|(1<<STEP_A)|(1<<STEP_B)|(1<<STEP_C)|(1<<STEP_D);
    DDRC &= ~((1<<BTN_LEFT)|(1<<BTN_RIGHT)|(1<<BTN_HORN));
    /* Enable pull‑up only for joystick button (BTN_HORN) – indicator buttons use external 1 kΩ to GND */
    PORTC  =  (1<<BTN_HORN);

    ADC_init();          /* already provided utility */
    Buzzer_InitPWM();    /* set up Timer0 for horn PWM */
    Servo_InitPWM();

    /* ---------- Task initialisation ---------- */
    tasks[0].state       = IND_idle;
    tasks[0].period      = IND_PERIOD;
    tasks[0].elapsedTime = 0;
    tasks[0].TickFct     = &Tick_Indicator;

    tasks[1].state       = HORN_off;
    tasks[1].period      = HORN_PERIOD;
    tasks[1].elapsedTime = 0;
    tasks[1].TickFct     = &Tick_Horn;

    tasks[2].state       = STEP_idle;
    tasks[2].period      = GCD_PERIOD;   /* fastest scheduler tick */
    tasks[2].elapsedTime = 0;
    tasks[2].TickFct     = &Tick_Stepper;

    tasks[3].state       = BEEP_idle;
    tasks[3].period      = 10;           /* 10 ms granularity fine for beep timing */
    tasks[3].elapsedTime = 0;
    tasks[3].TickFct     = &Tick_Beep;

    tasks[4].state       = 0;
    tasks[4].period      = 20;   // update every 20ms
    tasks[4].elapsedTime = 0;
    tasks[4].TickFct     = &Tick_Servo;

    TimerSet(GCD_PERIOD);
    TimerOn();

    while(1) { /* all work done in TimerISR */ }

    return 0;
}