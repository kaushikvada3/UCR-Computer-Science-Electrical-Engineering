#include <avr/io.h>
#include <avr/interrupt.h>
//#include <avr/signal.h>
#include <util/delay.h>
#include <stdlib.h>   // for itoa()

#ifndef HELPER_H
#define HELPER_H

//Functionality - finds the greatest common divisor of two values
//Parameter: Two long int's to find their GCD
//Returns: GCD else 0
unsigned long int findGCD(unsigned long int a, unsigned long int b)
{
	unsigned long int c;
	while(1){
		c = a % b;
		if( c == 0 ) { return b; }
		a = b;
		b = c;
	}
	return 0;
}

unsigned char SetBit(unsigned char x, unsigned char k, unsigned char b) {
   return (b ?  (x | (0x01 << k))  :  (x & ~(0x01 << k)) );
              //   Set bit to 1           Set bit to 0
}

unsigned char GetBit(unsigned char x, unsigned char k) {
   return ((x & (0x01 << k)) != 0);
}

 
/* 7‑segment glyph lookup and helper are defined in main.cpp */
extern int nums[];
void outNum(int num);

/* ---------- Minimal USART0 helper for 9600‑baud debug ---------- */
static inline void USART_Init_9600(void) {
  /* 9600 baud @16 MHz → UBRR = 103 */
  UBRR0H = (unsigned char)(103 >> 8);
  UBRR0L = (unsigned char)103;
  UCSR0B = (1 << TXEN0);                         /* enable transmitter */
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);        /* 8‑bit, no parity, 1 stop */
}

static inline void USART_TxChar(char c) {
  while (!(UCSR0A & (1 << UDRE0))) { }           /* wait for buffer empty */
  UDR0 = c;
}

static inline void USART_TxString(const char *s) {
  while (*s) USART_TxChar(*s++);
}

static inline void USART_TxInt(int val) {
  char buf[8];
  itoa(val, buf, 10);
  USART_TxString(buf);
}

#endif /* HELPER_H */