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
    typedef float (*convert_t)(float); //prototype for conversion functions

    adc(void) { init(); }

    float get_voltage(u8 channel);
    float get_value(u8 channel);

    void reg_converter(u8 channel, convert_t converter);

private:
    static const u8 num_channels = 7;
    static const u8 num_conversions = 10;
    static const pindef adcpins;
    volatile u16 conversion_array[num_conversions * num_channels];

    convert_t converters[num_channels];

    void init();
};

#endif /* ADC_HPP_ */
