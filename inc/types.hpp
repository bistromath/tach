/*
 * types.hpp
 *
 *  Created on: Nov 11, 2008
 *      Author: bistromath
 */

#ifndef TYPES_HPP_
#define TYPES_HPP_

#define VERSION_MAJOR 1
#define VERSION_MINOR 38

//putting these here until i find out something better to do with them
#define BIT_0 0x01
#define BIT_1 0x02
#define BIT_2 0x04
#define BIT_3 0x08
#define BIT_4 0x10
#define BIT_5 0x20
#define BIT_6 0x40
#define BIT_7 0x80
#define BIT_8 0x100
#define BIT_9 0x200
#define BIT_10 0x400
#define BIT_11 0x800
#define BIT_12 0x1000
#define BIT_13 0x2000
#define BIT_14 0x4000
#define BIT_15 0x8000
#define CALSTEPS 8
#define RAM_RESIDENT __attribute__ ((long_call, section (".ramfunc")))

typedef struct {
	u16 r;
	u16 g;
	u16 b;
	u16 y;
} rgby_t;

struct hsv_t {
	u16 hue;
	u16 saturation;
	u16 value;
};

//this must ONLY BE USED FOR ZYLINK connectivity to Z90s!
//just a note here, when you do get Zylink working, watch those bools
//i don't remember if IAR packed them or not, and that will fux up Zylink
//don't think they're packed
struct Z90STATE {
  struct hsv_t color; //desired HSV output
  u8 mode; //0 for white, 1 for gel, 2 for color
  u8 preset;
  u16 whitemode_colortemp;
  u16 colortemp;
  s16 green;
  bool power; //1 for on, 0 for off
  bool zylink; //1 for link, 0 for off
  u8 channel; //ZyLink channel
  u8 address; //ZyLink address
//  struct BLINKENLIGHTS lights;
};

//see above. only used for transmitting Z50 data.
struct Z50STATE {
  struct hsv_t hsv;
  bool power;
  bool colormode;
  bool colortemp;
};

struct tempco_t {
	float a;
	float b;
	float c;
};

struct lz4tempco_t {
	struct tempco_t r;
	struct tempco_t g;
	struct tempco_t b;
	struct tempco_t y;
}; //LED temperature coefficients to perform temperature compensation

struct xyz_t { //does this need to be floating-point math, or can you just use u16?
	float x;
	float y;
	//float z;
};

struct uv_t {
	float u;
	float v;
};

typedef struct XYZ_t {
	float X;
	float Y;
	float Z;
} XYZ_t;

struct led_t {
	struct XYZ_t r;
	struct XYZ_t g;
	struct XYZ_t b;
	struct XYZ_t y;
};

struct pindef {
	GPIO_TypeDef *port;
	u16 pin;
};

typedef enum { DisplayBrightness_Low = 0, DisplayBrightness_High = 0xFF } DisplayBrightness_TypeDef;

typedef enum { IS3Mode_Off = 0, IS3Mode_White=1, IS3Mode_Color=2, IS3Mode_DMX=3 } IS3Mode_TypeDef;

typedef enum { IS3Preset_Autosave = 0, IS3Preset_1 = 1, IS3Preset_2 = 2 } IS3Preset_TypeDef;

struct IS3State {
	//this struct describes the complete state of the IS3, suitable for providing output over Zylink or loading from memory.
	//button positions are NOT included here, nor are display outputs; those determine, and are determined by, this state.
	//DMX mode should ONLY be signalled from the DMX switch input.
	IS3Mode_TypeDef mode;
	u16 colortemp;
	s16 duv; //have to translate that into a float somehow
	u16 brightness;
	struct hsv_t colormode_color;
	u16 colormode_colortemp;

	bool zylink_power;
	u8 zylink_channel;

	bool units; //are you in Regular or Extra-Special units mode?
	u16 diffusion; //diffusion level
};

#endif /* TYPES_HPP_ */
