/*
 * spi.cpp
 *
 *  Created on: Apr 8, 2009
 *      Author: bistromath
 */

extern "C" {
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
}
#include "spi.hpp"

void spi::init(void)
{
	//we'll set up DMA transfers that are flagged to start in an interrupt. we can use DMA2 for these transfers.
	//so it goes like this.
	//FIFOP going up triggers an interrupt that says HEY YOU GOT DATA YO
	//the interrupt sets up the DMA transfer from the SPI RX to the circular RX buffer (doesn't have to be circular)
	//the DMA complete interrupt sets a "HEY YOU GOT DATA YO" flag for the main loop

	//how do we manage RX buffers? By providing one in the constructor for the DMA to use as a dumping ground.
	//can't be a circular buffer though because DMA will not manage the construct. not a problem.

	SPI_InitTypeDef spiinit;
	GPIO_InitTypeDef gpioinit;



	spiinit.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	spiinit.SPI_Mode = SPI_Mode_Master;
	spiinit.SPI_DataSize = SPI_DataSize_8b;
	spiinit.SPI_CPOL = SPI_CPOL_Low;
	spiinit.SPI_CPHA = SPI_CPHA_1Edge;
	spiinit.SPI_NSS = SPI_NSS_Soft;
	spiinit.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16; //TODO: this could give up to 18MHz on the SPI bus! if something barfs THIS IS WHY
	spiinit.SPI_FirstBit = SPI_FirstBit_MSB;
	spiinit.SPI_CRCPolynomial = 0x1021;

	SPI_Init(spidev, &spiinit);

	gpioinit.GPIO_Speed = GPIO_Speed_50MHz;


	switch((u32)spidev) {
	case SPI1_BASE:
		gpioinit.GPIO_Mode = GPIO_Mode_AF_PP;
		gpioinit.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
		GPIO_Init(GPIOA, &gpioinit);
		gpioinit.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		gpioinit.GPIO_Pin = GPIO_Pin_6;
		GPIO_Init(GPIOA, &gpioinit);
		break;
	case SPI2_BASE:
		gpioinit.GPIO_Mode = GPIO_Mode_AF_PP;
		gpioinit.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_15;
		GPIO_Init(GPIOB, &gpioinit);
		gpioinit.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		gpioinit.GPIO_Pin = GPIO_Pin_14;
		GPIO_Init(GPIOB, &gpioinit);
		break;
	default:
		break;
	}

	gpioinit.GPIO_Pin = cs_pin.pin;
	gpioinit.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(cs_pin.port, &gpioinit);

	chip_select(false);

	//here you gotta set up the DMA receive interrupt stuff

	SPI_Cmd(spidev, ENABLE);
}

u8 spi::send(u8 data) {
//you really do want to use DMA eventually, but FUCK IT. just get it working.
	//chip_select(true);
	//SPI_NSSInternalSoftwareConfig(spidev, SPI_NSSInternalSoft_Set);
	while(!SPI_I2S_GetFlagStatus(spidev, SPI_I2S_FLAG_TXE)); //make sure it's clear to send
	SPI_I2S_SendData(spidev, data);
	while(!SPI_I2S_GetFlagStatus(spidev, SPI_I2S_FLAG_RXNE)); //wait for it to clock out
	//chip_select(false);
	//SPI_NSSInternalSoftwareConfig(spidev, SPI_NSSInternalSoft_Reset);
	return SPI_I2S_ReceiveData(spidev); //return the data sent back on MISO
}
