#ifndef SERIAL_H
#define SERIAL_H

#include <avr/io.h>

void USART_init(unsigned int ubrr);
void USART_transmit(unsigned char data);
void USART_print(const char* str);
void USART_print_hex(uint32_t value);

#endif