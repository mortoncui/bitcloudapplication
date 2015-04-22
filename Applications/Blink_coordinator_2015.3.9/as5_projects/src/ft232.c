/*
 * ft232.c
 *
 * Created: 2014/12/26 21:27:03
 *  Author: Administrator
 */ 
//#define FOSC 8000000//Clock Speed
//#define BAUD 9600
//#define MYUBRR (FOSC/16/BAUD-1)
#undef F_CPU
#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
//引入 interrupt.h， sei()可以使用了
#include <avr/interrupt.h>
//#include <ft232.h>
void Usart_init( unsigned int ubrr){
/* Set baud rate */
UBRR1H = (unsigned char)(ubrr>>8);
UBRR1L = (unsigned char) ubrr;
/* Enable receiver and transmitter */
UCSR1B = (1<<RXEN1)|(1<<TXEN1);
/* Set frame format: 8data, 2stop bit */
UCSR1C = (1<<USBS1)|(3<<UCSZ10);
} // USART_Init
ISR(USART1_RX_vect)
{
 unsigned char ReceivedByte;
 // Fetch the received byte value into the variable "ByteReceived"
 ReceivedByte = UDR1;
 // Echo back the received byte back to the computer
 UDR1 = ReceivedByte - 32;
}
void Usart_intenable(void)
{
	UCSR1B |= (1<<RXCIE1);
	// Enable the Global Interrupt Enable flag
	//so that interrupts can be processed
	sei();
}	


