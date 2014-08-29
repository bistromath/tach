/*
 * adc.hpp
 *
 *  Created on: Apr 21, 2009
 *      Author: bistromath
 */

#ifndef ADC_HPP_
#define ADC_HPP_

extern "C" {
#include "stm32f10x.h"
#include <stm32f10x_conf.h>
#include <string.h>
}

#include "types.hpp"

class adc {
public:
	adc(void) { init(); }

	void get_temps(float temps[7]);

	const static float temp_coeffs[7][2];


private:
	static const u8 num_channels = 7;
	static const u8 num_conversions = 100;

	static const pindef adcpins;

	volatile u16 conversion_array[num_conversions * num_channels];

	void init();
};

#endif /* ADC_HPP_ */
