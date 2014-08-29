/*
 * serial.h
 *
 *  Created on: Feb 12, 2009
 *      Author: bistromath
 *
 *      implements a buffered serial port interface using circular_buffer.hpp
 *      suitable for a debug interface or a cal interface
 *      sort of shitty because of the packet framing method
 */

#ifndef SERIAL_H_
#define SERIAL_H_
//#include "circular_buffer.hpp"

extern "C" {
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
}

class serial {
public:
	serial();
	serial(USART_TypeDef* usart_num, u16 baudrate, volatile char *rxbuffer, u16 rxbuflen) : usart(usart_num), rxbuf(rxbuffer, rxbuflen) { init(usart_num, baudrate); }
	void send(char *buf, u8 len);
	void receive() {
		//this is a problem: if the cal data contains a \r it will sentenceify in the wrong place
		//but it should NACK due to incorrect length, so I guess you can just hit recal for now
		//proper response is to peek at the $ and length.... yeah that'll do
		//except that '$' is ALSO A VALID CHARACTER
		//arrrrrggghhhhhhh

		rxbuf.push(usart->DR);
		if(rxbuf.peeklast()=='\r') {
			if(rxbuf.peek(1) == rxbuf.unread() - 4) sentenceready=1;
		}
	}
	void read(volatile char *buffer, u16 length);
	bool data_ready() {return rxbuf.unread()!=0;}
	bool sentence_ready() {return sentenceready;}
	u16 read_sentence(volatile char *buffer) {
		int i = 0;
		while(!rxbuf.isempty()) {
			rxbuf.pop(buffer);
			buffer++;
			i++;
		}
		sentenceready = 0;
		return i;
	}

	void irq_handler();

private:
	void init(USART_TypeDef* usart_num, u16 baudrate);
	USART_TypeDef* usart;
	circular_buffer rxbuf;
	volatile bool sentenceready;
	bool newsentence;
	int sentlen;
};




#endif /* SERIAL_H_ */
