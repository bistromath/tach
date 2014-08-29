//button.hpp
//defines a button class to manage pushbuttons on the IS3/STM32

/*
 *  here's what you thought the other night. the button class can include a private array of 8 (or so) integers. this is a static array.
	this array keeps track of buttons that are held down by incrementing the array elements.

	initializing a button object makes it an input (pull-up) and sets up an edge interrupt for when the line is pulled down.
	the interrupt allocates a tick_callback object

	calls a static function which polls the GPIOs (how to determine which to poll?) and increments the associated button timers
	for every

	you know what, look. Look. you're only going to initialize these once, so why not make them static? Just make a single fucking map of the
	buttons you want to poll, and initialize and handle them as a group. There. Done.

	the more i think about this, the more i think maybe polling is a better option than interrupts. the interrupt controller only allows one
	port per pin to trigger the interrupt; e.g., if I use C4 and B4 as buttons, only ONE of them can trigger the interrupt. this isn't
	sustainable and limits your options. plus i don't think you had this in mind when you routed it. so let's move to polling.


*/

extern "C" {
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
}

#include "tick_callback.hpp"

typedef enum
{
	Button_Mode_White = 0,
	Button_Mode_Color,
	Button_Disp_Brightness,
	Button_Zylink,
	Button_Units,
	Button_Preset_1,
	Button_Preset_2
} Button_Name_TypeDef;


class buttons {
public:

	buttons(); //constructor initializes all pins

	bool read(Button_Name_TypeDef button) {
		if(button == Button_Mode_White || button == Button_Mode_Color) return !GPIO_ReadInputDataBit(button_list[button].port, button_list[button].pin);
		else {
			bool result = button_array[button];
			button_array[button] = false; //reading it says Yes I've Seen The Last Raised Transition
			return result;
		}
	}

	bool read_raw(Button_Name_TypeDef button) {
		return !GPIO_ReadInputDataBit(button_list[button].port, button_list[button].pin);
	}

	bool read_held(Button_Name_TypeDef button) {
		bool result = hold_down_array[button];
		hold_down_array[button] = false;
		return result;
	}

	static void tick_callback_handler(); //polls GPIOs and updates timers


private:
	//these static vars are defined in the cpp
	static const pindef button_list[];
	static const u8 num_buttons;
	static const u16 hold_down_threshold;

	volatile static bool button_array[7];
	volatile static bool ignore_edge_array[7];
	volatile static bool hold_down_array[7];
	volatile static u16 timers[7];
	tick_callback button_callback; //do not initialize yet

	void init(void) { //initializes the button_callback object to count button-down time
		button_callback.set_delay(50);
		button_callback.set_callback(tick_callback_handler);
		button_callback.start();
	}

	void deinit(void) { //deinitializes the button_callback object
		button_callback.set_callback(0);
	}
};
