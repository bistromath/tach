/*
 * tick_callback.hpp
 *
 *  Created on: Nov 6, 2008
 *      Author: bistromath
 *
 *      Overloads the systick class to add support for function callbacks. Use is pretty obvious, but a warning:
 *      EVERYTHING YOU DO IN TICK CALLBACKS IS EXECUTED WITHIN THE INTERRUPT. DO NOT DO ANYTHING HEAVY IN INTERRUPTS.
 *
 *      Example:
 *      void blinker_fn(void) { //blink an LED or toggle something or do something light }
 *
 *      tick_callback blink(blinker_fn, 100); //call blinker_fn every 100ms
 *      blink.start(); //called from parent class
 *      blink.stop();
 */

#ifndef TICK_CALLBACK_HPP_
#define TICK_CALLBACK_HPP_

#include "systick.hpp"
#include "types.hpp"

extern "C" {
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
}

class tick_callback : public systick {
public:
	tick_callback() : systick(), callback(0), callback_delay(0) { }
	tick_callback(void (*func)(void), u16 delay) { tick_callback(); set_callback(func); set_delay(delay); }

	void set_callback(void (*func)(void)) { callback = func; }
	void set_delay(u16 delay_ms) { callback_delay = delay_ms; }
	void irqhandler() RAM_RESIDENT;

private:
	void (*callback)(void);
	u16 callback_delay;
};


#endif /* TICK_CALLBACK_HPP_ */
