/*
 * stepper.hpp
 *
 *  Created on: Jan 28, 2010
 *      Author: bistromath
 */

#ifndef STEPPER_HPP_
#define STEPPER_HPP_

extern "C" {
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
}

#include "types.hpp"
#include "tick_callback.hpp"

class stepper {
public:
	stepper(); //inits output pins and enable pins, seeks to home, sets loop error, starts loop timer
	void home(); //performs a home seek to ensure the needle is against the CCW stop
	void seek(u16 position); //the userspace function to set the setpoint for the needle
	static const u16 max_position = 945; //315 degrees x 1/3 degree per step
    volatile static u16 get_position(void) { return current_position; }
	static void isr_handler(void);


private:
	static void step_fwd(void);
	static void step_back(void);
	static void commutate(void);

	volatile static u8 step_counter; //position counter to wrap around the commutation cycle (6 steps)
	volatile static u16 current_position; //might not need this, just the current step position of the motor
	volatile static s16 current_rate;
	volatile static bool current_direction;
	static u16 target_position;

	static const bool commutation_chart[4][6];
	static const u16 rate_steps[];
    static const u16 prescaler;
	static const pindef outputs[4], enables[4];
};

#endif /* STEPPER_HPP_ */
