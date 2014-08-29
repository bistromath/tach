/*
 * tick_callback.cpp
 *
 *  Created on: Nov 6, 2008
 *      Author: bistromath
 */

#include "tick_callback.hpp"
#include "systick.hpp"
#include "types.hpp"

void tick_callback::irqhandler(void)
{
	systick::irqhandler();
	if(callback && (get_elapsed_ms() > callback_delay)) {
		callback();
		reset();
	}
}
