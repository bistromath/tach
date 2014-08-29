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

//how should commutation order be set up? in fact, what IS the comm order?

//service loop time:
//max speed 600d/sec
//steps are 1/3d
//1800 steps/sec max speed
//loop will have to run at LEAST that fast
//let's run the loop at 3kHz to start
//ideally everything should be set up so as to be loop-speed-agnostic
//actually, let's run at 1000Hz to start.
//make sure to limit your gains so that the terms don't result in a calculated speed of more than 600d/s

//it's a control loop centered around position, in counts. but velocity and acceleration must also be regulated.
//you can regulate initial pointer velocity using the P gain. the P gain must never drive the pointer faster than 200Hz.
//but it can certainly drive slower. this gain can be set pretty high, as it will saturate at 200Hz.

//you know, you could use the loop to just set the next commutation time, and have the tick_callback calculate the next required comm time
//and set itself to call back when it's ready. that way you could set the loop to run as fast as it needs to and it would take care of itself.
//set a minimum callback time to limit velocity, and a maximum callback time to keep it idling.

//this would save you from having to callback at the maximum frequency all the time.
//however, you would have to calculate your loop error in a time-agnostic way. it would help if you kept track of your units.

//P gain is in units counts/sec. it can never exceed 200.
//I gain is in units counts/sec/sec. it can never exceed
//D gain is in units counts. it can never exceed

//CALCULATING P GAIN: this is the PROP GAIN. it must never create a driving speed of more than ~200Hz, or 1/5 at 1000Hz.
//CALCULATING I GAIN: this is the INTEGRAL GAIN. it must never add error in steps of greater than

class stepper {
public:
	stepper(); //inits output pins and enable pins, seeks to home, sets loop error, starts loop timer

	void home(); //performs a home seek to ensure the needle is against the CCW stop

	void seek(float position); //the userspace function to set the setpoint for the needle, in 0-1

	volatile static u16 get_position(void) { return current_position; }

	static void isr_handler(void);


private:
	//the values here are initialized in the CPP file. later they will be consts, leaving them variable now so you can tune.

	static void step_fwd(void);
	static void step_back(void);
	static void commutate(void);

	volatile static u8 step_counter; //position counter to wrap around the commutation cycle (6 steps)
	volatile static u16 current_position; //might not need this, just the current step position of the motor
	volatile static s16 current_rate;
	volatile static bool current_direction;
	static u16 target_position;

	static const u16 max_position = 960; //315 degrees x 1/3 degree per step

	static const bool commutation_chart[4][6];

	static const u16 rate_steps[];

	static const pindef outputs[4], enables[4];
};

#endif /* STEPPER_HPP_ */
