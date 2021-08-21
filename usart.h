#ifndef USART_H_
#define USART_H_

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdbool.h> //dodane zeby dzialala funkcja bool usart_set_baudrate

bool usart_set_baudrate	(USART_t *, uint32_t, uint32_t);
void setUpSerial();
void sendChar(char);
void sendCharBT(char);
void sendString(char *);
void sendStringBT(char *);
int uart_putchar (char, FILE *);
int uart_putcharBT (char, FILE *);
char usart_receiveByte();
char usart_receiveByteBT();
int uart_getchar(FILE *);
int uart_getcharBT(FILE *);

#endif /* USART_H_ */




