/*
 * spi.hpp
 *
 *  Created on: Apr 8, 2009
 *      Author: bistromath
 *
 *      SPI class
 */

#ifndef SPI_HPP_
#define SPI_HPP_

extern "C" {
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include <stdlib.h>
#include <string.h>
}
#include "types.hpp"

class spi
{
public:
	spi(SPI_TypeDef *bus, pindef enable) : spidev(bus),
										   cs_pin(enable)
										   { init(); }

	u8 send(u8 data); //if you want to write, just throw away the result! to read, send 0x00 or use receive()
	u8 receive(void) { return send(0x00); }

	void chip_select(bool select) {
		if(select) GPIO_ResetBits(cs_pin.port, cs_pin.pin);
		else GPIO_SetBits(cs_pin.port, cs_pin.pin);
	}

protected:
	void init();
	SPI_TypeDef *spidev; //duh
	pindef cs_pin; //the chip select pin

private:

};


#endif /* SPI_HPP_ */
