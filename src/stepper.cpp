/*
 * stepper.cpp
 *
 *  Created on: Jan 28, 2010
 *      Author: bistromath
 */

extern "C" {
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
}

#include "stepper.hpp"

volatile u8 stepper::step_counter;
u16 stepper::target_position;
volatile u16 stepper::current_position;
volatile s16 stepper::current_rate;
volatile bool stepper::current_direction;
//const s16 stepper::rate_steps[8] = {0, 235, 350, 425, 475, 535, 575, 600}; //the acceleration steps

//this is the rate steps in timer ticks instead of Hz, assuming a 18MHz clock rate.
//const s16 stepper::rate_steps[8] = { 38297, 38297, 25714, 21176, 18947, 16822, 15652, 15000 };
//this version is based on keeping the acceleration rate per FULL STEP, rather than third-step.
//const u16 stepper::rate_steps[20] = { 38297, 38297, 32855, 28731, 25714, 23591, 22149, 21176, 20285, 19570, 18947, 18185, 17466, 16822, 16347, 15963, 15652, 15399, 15187, 15000 };

const u16 stepper::rate_steps[] = { 57447, 38298, 32927, 28877, 25714, 24000, 22500, 21176, 20377, 19636, 18947, 18182, 17476, 16822, 16413, 16024, 15652, 15429, 15211, 15000, 15000, 15000 };
const u16 NUM_STEPS=22;

//this version is based on full-steps at the FSS, and requires a 18MHz clock rate.
//const s16 stepper::rate_steps[8] = { 36000, 36000, 36000, 36000, 36000, 36000, 36000, 36000 };

const bool stepper::commutation_chart[4][6] = {
		{1, 1, 1, 0, 0, 0},
		{0, 0, 1, 1, 1, 0},
		{1, 0, 0, 0, 1, 1},
		{0, 0, 1, 1, 1, 0}
};

stepper::stepper() {
	GPIO_InitTypeDef gpioinit;

	for(int i = 0; i < 4; i++) {
		gpioinit.GPIO_Mode = GPIO_Mode_Out_PP;
		gpioinit.GPIO_Pin = outputs[i].pin;
		gpioinit.GPIO_Speed = GPIO_Speed_50MHz;

		GPIO_Init(outputs[i].port, &gpioinit);
	}

	for(int i = 0; i < 4; i++) {
		gpioinit.GPIO_Mode = GPIO_Mode_Out_PP;
		gpioinit.GPIO_Pin = enables[i].pin;
		gpioinit.GPIO_Speed = GPIO_Speed_50MHz;

		GPIO_Init(enables[i].port, &gpioinit);
	}

	for(int i = 0; i < 4; i++)
	{
		GPIO_ResetBits(outputs[i].port, outputs[i].pin);
		GPIO_SetBits(enables[i].port, enables[i].pin);
	}

	//set up a timer to be ready to do commutation for us.
	//preload it, set for countdown, use the ISR to do the commutating.

	TIM_TimeBaseInitTypeDef tim1init;
	tim1init.TIM_ClockDivision = TIM_CKD_DIV1;
	tim1init.TIM_CounterMode = TIM_CounterMode_Up;
	tim1init.TIM_Period = rate_steps[0];
	tim1init.TIM_Prescaler = 1;
	tim1init.TIM_RepetitionCounter = 0;

	TIM_TimeBaseInit(TIM1, &tim1init);
	TIM_SetCounter(TIM1, 0);
	//now configure the interrupts (on update)
	TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
	//TIM_UpdateDisableConfig(TIM1, ENABLE);
	TIM_UpdateRequestConfig(TIM1, TIM_UpdateSource_Regular);

	NVIC_InitTypeDef nvicinit;
	nvicinit.NVIC_IRQChannel = TIM1_UP_IRQn;
	nvicinit.NVIC_IRQChannelCmd = ENABLE;
	nvicinit.NVIC_IRQChannelPreemptionPriority = 0;
	nvicinit.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvicinit);

	TIM_Cmd(TIM1, ENABLE);

	current_rate = 0;
	home();
}

void stepper::step_fwd(void) {
	step_counter++;
	if(step_counter > 5) step_counter = 0;

	if(current_position < max_position) current_position++;

	commutate();
}

void stepper::step_back(void) {
	step_counter--;
	if(step_counter > 6) step_counter = 5; //u8 wraps to 255

	if(current_position > 0) current_position--;

	commutate();
}

void stepper::commutate(void) {
	for(int i = 0; i < 4; i++) {
		if(commutation_chart[i][step_counter] == 1) {
			GPIO_SetBits(outputs[i].port, outputs[i].pin);
		}
		else {
			GPIO_ResetBits(outputs[i].port, outputs[i].pin);
		}
	}
}

void stepper::home(void) {
	extern void delay_ms(u16);
	TIM_Cmd(TIM1, DISABLE);
	TIM_SetCounter(TIM1, 0);
	//delay_ms(100);
	current_position = max_position;
	current_rate = 0;
	seek(0);
	TIM_PrescalerConfig(TIM1, 2, DISABLE); //move at half rate
	TIM_Cmd(TIM1, ENABLE);
	while(current_position > 0);
	TIM_Cmd(TIM1, DISABLE);
	TIM_SetCounter(TIM1, 0);
	TIM_PrescalerConfig(TIM1, 1, DISABLE);
//	delay_ms(200);
	TIM_Cmd(TIM1, ENABLE);
}

void stepper::seek(float position) {
	if(position > 1.0) position = 1.0;
	if(position < 0.0) position = 0.0;
	target_position = position * float(max_position);
}

void stepper::isr_handler(void) {
	extern void fail();
	//this gets called by the timer ISR.

	bool target_direction = (s32(target_position) - s32(current_position)) > 0; //calc the direction we're planning to go
	//but the target direction isn't necessarily the direction we're heading in.
	//we need to know that direction in order to find out where to go

	//so we find it.
	if(current_rate == 0) { //if we're stopped, we know we want to move in our target direction
		current_direction = target_direction;
	}

	//if our target direction is the same as our current direction, we wish to accelerate.
	if(current_direction == target_direction) {
		if(current_rate < NUM_STEPS-1)
			current_rate++; //limit acceleration so we don't run off the end of the array.
	}
	else {
		if(current_rate == 0) fail(); //i don't think it can reach this but we don't want to take that chance.
		current_rate--; //otherwise, we are decelerating.
	}

	u16 distance_to_go = abs(s16(target_position) - s16(current_position));
	if(distance_to_go < NUM_STEPS-1 && current_direction == target_direction) { //it's time to decelerate! we're almost there!
		if(current_rate > distance_to_go) current_rate = distance_to_go;
	}

	//commutate the motor
	if(current_rate != 0) {
		if(current_direction) step_fwd();
		else step_back();
	}

	s16 preload = rate_steps[current_rate]/2; //TODO FIXME DIV2 BECAUSE CLK IS 9MHz NOW

	//so we calculate the preload for the timer ISR, based on that rate
	//stop the timer
	TIM_Cmd(TIM1, DISABLE);

	//load the preload
	TIM_SetAutoreload(TIM1, preload);

	//reset the timer
	TIM_SetCounter(TIM1, 0);

	//start the timer
	TIM_Cmd(TIM1, ENABLE);

}
