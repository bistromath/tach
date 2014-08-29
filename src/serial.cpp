/*
 * serial.cpp
 *
 *  Created on: Feb 12, 2009
 *      Author: bistromath
 *
 *      a little serial driver to provide debug output, etc
 */

extern "C" {
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "misc.h"
}

#include "circular_buffer.hpp"
#include "serial.hpp"

void serial::init(USART_TypeDef* usart_num, u16 baudrate)
{
	//initialize the peripheral (and its clock!) as well as the DMA controller
	//ok clock is taken care of in vector
	USART_InitTypeDef usartinit;
	USART_ClockInitTypeDef usartclockinit;
	GPIO_InitTypeDef GPIO_InitStructure;

	usartinit.USART_BaudRate = baudrate;
	usartinit.USART_WordLength = USART_WordLength_8b;
	usartinit.USART_StopBits = USART_StopBits_1;
	usartinit.USART_Parity = USART_Parity_No;
	usartinit.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	usartinit.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

	USART_ClockStructInit(&usartclockinit);
	usartclockinit.USART_Clock = USART_Clock_Disable; //async
	usartclockinit.USART_LastBit = USART_LastBit_Disable;

	USART_Init(usart_num, &usartinit);
	USART_ClockInit(usart_num, &usartclockinit);
	USART_ITConfig(usart_num, USART_IT_RXNE, ENABLE);
	USART_ITConfig(usart_num, USART_IT_ORE, ENABLE);
	//USART_ITConfig(usart_num, USART_IT_FE, ENABLE);
	USART_ITConfig(usart_num, USART_IT_TC, DISABLE);
	USART_Cmd(usart_num, ENABLE);

	NVIC_InitTypeDef nvicinit;
	//NVIC_StructInit(&nvicinit);
	nvicinit.NVIC_IRQChannelPreemptionPriority = 0;
	nvicinit.NVIC_IRQChannelSubPriority = 0;
	nvicinit.NVIC_IRQChannel = USART2_IRQn;
	nvicinit.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvicinit);

	GPIO_PinRemapConfig(GPIO_Remap_USART2, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

}

//simple blocking implementation for now because really it's debug output anyway
//gonna throw garbage for odd-length outputs at the end
void serial::send(char *str, u8 len)
{
	while((len--) > 0) {
		while(USART_GetFlagStatus(usart, USART_FLAG_TXE) == RESET);
		USART_SendData(usart, *(str++));
		while(USART_GetFlagStatus(usart, USART_FLAG_TC) == RESET);
		USART_ClearFlag(usart, USART_FLAG_TC);
	}
}

void serial::irq_handler() {
	if(USART_GetFlagStatus(usart, USART_FLAG_ORE)) {
		USART_ClearFlag(usart, USART_FLAG_ORE);
		USART_ClearITPendingBit(usart, USART_IT_ORE);
	}

	USART_ClearITPendingBit(usart, USART_IT_RXNE);
	//USART_ClearITPendingBit(USART2, USART_IT_TC);
	USART_ClearFlag(usart, USART_FLAG_RXNE);
	//USART_ClearFlag(USART2, USART_FLAG_TC);
	receive();
}
