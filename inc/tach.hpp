/*
 * tach.hpp
 *
 *  Created on: Jan 30, 2010
 *      Author: bistromath
 *
 *      All this class does is set up timer 3, CC1 as a tachometer input, and maintain a running average of numavg samples.
 *      It's DMA-enabled if you look in the .cpp, so there's no interaction until asked for an average.
 *      The interrupt is only for when the timer overflows, indicating underrange RPM.
 *
 */

#ifndef TACH_HPP_
#define TACH_HPP_

extern "C" {
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
}

#include "types.hpp"
//#include "tick_callback.hpp"

class tach {
public:
	tach();

	u16 get_rpm(void); //calc averages and return RPM

	void irq_handler(void); //called by the CC interrupt on TIM3

private:
	const static uint16_t numavg = 5;
	uint16_t averages[tach::numavg];
    uint32_t ticksperminute;
    uint32_t blanking;
};


#endif /* TACH_HPP_ */
