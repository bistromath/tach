/*
 * systick.hpp
 * Implements a class to handle the system tick timer and register counters to calculate time delays
 *
 * Known issues: something about tick_callback (see tick_callback.hpp) being stopped results in a hard fault. likely dereferencing a null ptr.
 *
 * Notes about this class: It depends on the creation of a global array of systick pointers (ie systick* array[10]) pointing to the objects in order to execute
 * their interrupt handlers. If you really wanted to, you could use function pointers to execute overloaded child class handlers instead of virtual functions.
 * This would save you ~3K of code space (unoptimized) but wouldn't be as clean. Then the array would be void (*func[10])(void) and the allocation function
 * would traverse the array of function pointers looking for unused ones. But if you do this, how does each function pointer point to a specific instance of the
 * systick class? i.e., how would the function pointer know it's operating on a specific systick if the function is static? How about I stick to virtual functions
 * for now. The call overhead appears to be pretty minimal. And from what I can tell function pointer calls take as much time as virtual function calls.
 *
 * Anyway, it provides a framework for measuring time and creating accurate delays based on the SysTick timer built into the STM32.
 * You can use it like this:
 * systick timer;
 * timer.init_hardware(); //only needs to be done for the first systick object initialized, should probably be part of the constructor
 * timer.start();
 *  (do something here)
 * timer.stop();
 * u16 time=timer.get_elapsed_us();
 *
 * Remember to declare a global array:
 * systick* systick_timers[10];
 *
 * And your systick interrupt should contain:
 * extern systick* systick_timers[];
 * for(int i = 0; i < 10; i++) if(systick_timers[i]) systick_timers[i]->irqhandler();
 *
 * You should see that it is very easy to create an interrupt-independent delay_ms/us function using this library. Unlike a fixed-spin delay function,
 * a delay using the SysTick timer is independent of time spent in interrupts, unless an interrupt is executing when the delay expires.
 *
 * Because of function call overhead calling systick::start()/systick::stop(), I wouldn't call this accurate for microsecond-level timing. Above 10-20us you're
 * probably okay.
 *
 * The virtual function/function pointer overhead is only incurred for timers that are enabled and should be pretty minimal.
 *
 * The irqhandler() is virtualized so that you can overload it in child classes. See tick_callback.hpp for an example.
 *
 * I'm using an array of systick* so that you can allocate timers dynamically without using malloc/new. If that's your thing feel free to rewrite it but
 * you'll probably have to traverse a linked list or something in the interrupt and I can't imagine that being good.
 *
 * Known issues: No error checking to see if allocating a timer from the array fails. init_hardware() not automagically called in the constructor.
 * Should probably #define the number of timers to allocate instead of hardcoding it.
 *
 * I have not profiled the interrupt with all 10 timers allocated. You might want to see how long it's taking to update them all, but I can't see it being more
 * than 5us or so and that's not much time every 1ms.
 *
 *  Created on: Nov 5, 2008
 *      Author: bistromath
 */

#ifndef SYSTICK_HPP_
#define SYSTICK_HPP_

//systick errors
#define SYSTICK_NO_ALLOC 2

extern "C" {
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
}
#include "types.hpp"

class systick {
public:
	systick();

	void init_hardware(); //the constructor should be smart enough to look to see if the systick is running and call this if it isn't. right now it isn't.

	virtual void irqhandler() RAM_RESIDENT; //declaring this virtual cost me 3K code space and 1K RAM for malloc. wtf?

	u8 alloc() __attribute__ ((warn_unused_result)); //yeah i want you to look at this all right don't ignore it although really i should throw but the overhead is insane
	void dealloc();

	void start() { us_counter = SysTick_GetCounter(); enabled = 1; }
	void stop() { enabled = 0; us_counter_end = SysTick_GetCounter(); }
	void reset() RAM_RESIDENT;

	u32 get_elapsed_ms() const RAM_RESIDENT;
	u32 get_elapsed_us() const;

	static const u8 numtimers = 10;

private:
	u16 SysTick_GetCounter() { return SysTick->VAL; }


	static const u8 numctrs = 10;
	static const u32 preload = 8000; //it's div8, so 8MHz, so 1ms would be 8000

	u32 ms_counter;
	u32 us_counter;
	u32 us_counter_end;
	bool enabled; //which ones are running
	bool allocated; //which ones are allocated
	u8 alloc_val;

	u16 delayctr; //delay counter for blocking msec-scale delays
};

#endif /* SYSTICK_HPP_ */
