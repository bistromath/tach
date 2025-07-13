/*
 * systick.cpp
 *
 *  Created on: Nov 6, 2008
 *      Author: bistromath
 */

extern "C" {
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "misc.h"
}

#include "systick.hpp"

systick* systick_timers[systick::numtimers]; //list of systick timers used for callbacks from systick interrupt
											 //declared global so the interrupt can see it
//i suppose you could make it a static member of the class now that global constructors work
extern void fail();

systick::systick()
{
	ms_counter = 0;
	us_counter = 0;
	us_counter_end = 0;
	enabled = 0;
	allocated = 0;
	if(alloc()) fail(); //well what do we do if it fails to allocate a timer?
}

void systick::init_hardware()
{
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);
	//SysTick_SetReload(preload);
	SysTick->LOAD = preload;
	//SysTick_CounterCmd(SysTick_Counter_Clear);
	SysTick->VAL = 0;
	SysTick->CTRL |= 0x0001;
	//SysTick_CounterCmd(SysTick_Counter_Enable);
	//SysTick_ITConfig(ENABLE);
	SysTick->CTRL |= 0x0002;
}

u8 systick::alloc()
{
	u8 i = 0;
	if(allocated) return 0;
	while (systick_timers[i] && (i < numctrs)) i++;
	if (i == numctrs) return SYSTICK_NO_ALLOC; //failed to allocate a new timer
	ms_counter = 0;
	us_counter = 0;
	us_counter_end = 0;
	enabled = 0;
	allocated = 1;
	alloc_val = i;
	systick_timers[i] = this;
	return 0;
}

void systick::dealloc()
{
	systick_timers[alloc_val] = 0;
	allocated = 0;
	enabled = 0;
}
//if you really want to make this fast, run it in RAM
//FIXME: this probably needs to be updated now that our clock has changed
u32 systick::get_elapsed_us() const
{
	static u32 i;
	if (enabled)
		i = SysTick->VAL;
	else
		i = us_counter_end;
	u32 real_us = us_counter;
	u32 real_ms = ms_counter;
	if (i > real_us) {
		real_us += 9000;
		real_ms--;
	}
	u32 val = (real_us - i) / 9;
	if (!allocated)
		return -1;
	else
		return (val + real_ms * 1000);
}

void systick::irqhandler()
{
	if (enabled) ms_counter++;
	delayctr++;
}

void systick::reset()
{
	ms_counter = 0;
	us_counter = SysTick->VAL;
	us_counter_end = 0;
}

u32 systick::get_elapsed_ms() const {
		if(!allocated) return -1;
		return ms_counter;
}
