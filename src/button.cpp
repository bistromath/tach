extern "C" {
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
}

#include "types.hpp"
#include "tick_callback.hpp"
#include "button.hpp"

const u16 buttons::hold_down_threshold = 16; //in 50ms increments, this corresponds to one second

volatile u16 buttons::timers[7];
volatile bool buttons::button_array[7];
volatile bool buttons::hold_down_array[7];
volatile bool buttons::ignore_edge_array[7];

buttons::buttons() {
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;

	for(int i = 0; i < num_buttons; i++) { //initialize GPIOs
		GPIO_InitStructure.GPIO_Pin = button_list[i].pin;
		GPIO_Init(button_list[i].port, &GPIO_InitStructure);
	}

	for(int i = 0; i < num_buttons; i++) { //set all timers to 0, should be done for you... but might as well
		timers[i] = 0;
		button_array[i] = false;
	}

	init();
}

void buttons::tick_callback_handler() {
	//read the pins, all of them, and update the timers of whichever ones are low
	for(int i=0; i < num_buttons; i++) {
		if(!GPIO_ReadInputDataBit(button_list[i].port, button_list[i].pin)) {
			timers[i] += 1;
			button_array[i] = false;
			if(timers[i] > hold_down_threshold) {
				ignore_edge_array[i] = true;
				hold_down_array[i] = true;
				timers[i] = 0;
			}
		} else { //that is, if the pin is high
			if(timers[i] > 0 && timers[i] < hold_down_threshold && ignore_edge_array[i] == false) button_array[i] = true;
			timers[i] = 0;
			ignore_edge_array[i] = false;
		}
	}
}
