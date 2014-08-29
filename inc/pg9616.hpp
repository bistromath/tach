/*
 * pg9616.hpp
 *
 *  Created on: Oct 8, 2008
 *      Author: bistromath
 *
 *  Defines a display class for the Univision PG-9616 OLED display
 */

extern "C" {
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include <string.h>
}

#include "i2c.hpp"

#ifndef PG9616_HPP_
#define PG9616_HPP_

#define FONT_SMALL 0
#define FONT_LARGE 1
#define FONT_LARGE_WIDTH 8
#define FONT_SMALL_WIDTH 5
#define FONT_LARGE_CHAR_SPACING 2
#define NUM_DISPLAYS 1
//#define TXBUFLEN 255

typedef enum { Font_Small, Font_Large } Display_FontSizeTypeDef;

class pg9616 : public i2c_device
{
public:
	pg9616(I2C_TypeDef* I2Cx) : i2c_device(I2Cx, 0x78) { i2c_device::init(); init(); }//additional obj initialization goes here

	void init(void); //initializes the display; may take other options

	void print(u8 display, const char *string, Display_FontSizeTypeDef size, u8 start_page, u8 start_col); //prints string on the display, left-justified
	void print(u8 display, signed long number, Display_FontSizeTypeDef size, u8 width, u8 start_page, u8 start_col); //prints a number on the display, left-justified
	void print_base(u8 display, signed long number, u8 base, Display_FontSizeTypeDef font, u8 width, u8 start_page, u8 start_col);
	void print(u8 display, float number, Display_FontSizeTypeDef size, u8 width, u8 start_page, u8 start_col, bool plusminus, u8 max_decimals=255);
	void Show_Pattern(u8 display, const unsigned char Data_Pointer[][2], unsigned char start_page, unsigned char start_col, unsigned char width);
	void showlogo(u8 display) { Show_Pattern(display, logo, 0, 42, 12); }
	void showname(u8 display) { Show_Pattern(display, logoname, 0, 23, 50); }
	void showsplash(u8 display) { Show_Pattern(display, logo, 0, 10, 12); Show_Pattern(display, logoname, 0, 35, 50); }
	//void print_digit(u8 digit, u8 start_page, u8 start_col);
	void set_brightness(u8 brightness) { select_all(); Set_Contrast_Control(brightness); select(0); }
	void update(u8 display);
	void update(void);
	void clear(u8 display) { memset(&printbuf[display][0][0], 0, 96*2); } //update(display); while(busy()); }
	void clear(void) { for(int i=0;i<NUM_DISPLAYS;i++) clear(i); }
    void enable(void) { select_all(); Set_Display_On_Off(1); select(0); }
    void disable(void) { select_all(); Set_Display_On_Off(0); select(0); }

protected:
	void select(u8 display);
    void select_all(void);
	void Write_Command(unsigned char Data)
	{
		txbuf[0] = 0x00;
		txbuf[1] = Data;
		send(2);
		//for(int i = 0; i < 0xFFF; i++); //give it 4us before sending next char
		while(busy());
	}

	void Write_Command(unsigned char *Data, u8 len) {
		txbuf[0] = 0x00;
		memcpy((u8 *)txbuf+1, Data, len);
		send(len+1);
		while(busy());
	}

	void Write_Data(unsigned char Data)
	{
		txbuf[0] = 0x40;
		txbuf[1] = Data;
		send(2);
		while(busy());
		//for(int i = 0; i < 0xFFF; i++); //see above
	}

	void Write_Data(unsigned char *Data, unsigned char len) //overload this to use DMA transfers?
	{
		txbuf[0] = 0x40;
		memcpy((u8 *)txbuf+1, Data, len);
		send(len+1);
		while(busy());
		//for(int i = 0; i < 0xFFF; i++); //see above
	}

	//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
	//  Instruction Setting
	//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
	void Set_Start_Column(unsigned char d)
	{
		//Write_Command(0x00+d%16);		// Set Lower Column Start Address for Page Addressing Mode
							//   Default => 0x00
		//Write_Command(0x10+d/16);		// Set Higher Column Start Address for Page Addressing Mode
							//   Default => 0x10, using 0x12 to shift RAM by 32 cols for (128 col controller) - (96 col display) in reverse (upside-down) mode
		u8 command[3];
		command[0] = 0x00+d%16;
		command[1] = 0x10+d/16;
		Write_Command(command, 2);
	}

	void Set_Addressing_Mode(unsigned char d)
	{
		//Write_Command(0x20);			// Set Memory Addressing Mode
		//Write_Command(d);			//   Default => 0x02
							//     0x00 => Horizontal Addressing Mode
							//     0x01 => Vertical Addressing Mode
							//     0x02 => Page Addressing Mode
		u8 command[3];
		command[0] = 0x20;
		command[1] = d;
		Write_Command(command, 2);
	}


	void Set_Column_Address(unsigned char a, unsigned char b)
	{
		//Write_Command(0x21);			// Set Column Address
		//Write_Command(a);			//   Default => 0x00 (Column Start Address)
		//Write_Command(b);			//   Default => 0x7F (Column End Address)

		u8 command[3];
		command[0] = 0x21;
		command[1] = a;
		command[2] = b;
		Write_Command(command, 3);
	}


	void Set_Page_Address(unsigned char a, unsigned char b)
	{
		//Write_Command(0x22);			// Set Page Address
		//Write_Command(a);			//   Default => 0x00 (Page Start Address)
		//Write_Command(b);			//   Default => 0x07 (Page End Address)

		u8 command[3];
		command[0] = 0x22;
		command[1] = a;
		command[2] = b;
		Write_Command(command, 3);
	}


	void Set_Start_Line(unsigned char d)
	{
		Write_Command(0x40|d);			// Set Display Start Line
							//   Default => 0x40 (0x00)
	}


	void Set_Contrast_Control(unsigned char d)
	{
		Write_Command(0x81);			// Set Contrast Control
		Write_Command(d);			//   Default => 0x7F
	}


	void Set_Charge_Pump(unsigned char d)
	{
		Write_Command(0x8D);			// Set Charge Pump
		Write_Command(0x10|d);			//   Default => 0x10
							//     0x10 (0x00) => Disable Charge Pump
							//     0x14 (0x04) => Enable Charge Pump
	}


	void Set_Segment_Remap(unsigned char d)
	{
		Write_Command(0xA0|d);			// Set Segment Re-Map
							//   Default => 0xA0
							//     0xA0 (0x00) => Column Address 0 Mapped to SEG0
							//     0xA1 (0x01) => Column Address 0 Mapped to SEG127
	}


	void Set_Entire_Display(unsigned char d)
	{
		Write_Command(0xA4|d);			// Set Entire Display On / Off
							//   Default => 0xA4
							//     0xA4 (0x00) => Normal Display
							//     0xA5 (0x01) => Entire Display On
	}


	void Set_Inverse_Display(unsigned char d)
	{
		Write_Command(0xA6|d);			// Set Inverse Display On/Off
							//   Default => 0xA6
							//     0xA6 (0x00) => Normal Display
							//     0xA7 (0x01) => Inverse Display On
	}


	void Set_Multiplex_Ratio(unsigned char d)
	{
		Write_Command(0xA8);			// Set Multiplex Ratio
		Write_Command(d);			//   Default => 0x3F (1/64 Duty)
	}


	void Set_Display_On_Off(unsigned char d)
	{
		Write_Command(0xAE|d);			// Set Display On/Off
							//   Default => 0xAE
							//     0xAE (0x00) => Display Off
							//     0xAF (0x01) => Display On
	}


	void Set_Start_Page(unsigned char d)
	{
		Write_Command(0xB0|d);			// Set Page Start Address for Page Addressing Mode
							//   Default => 0xB0 (0x00)
	}


	void Set_Common_Remap(unsigned char d)
	{
		Write_Command(0xC0|d);			// Set COM Output Scan Direction
							//   Default => 0xC0
							//     0xC0 (0x00) => Scan from COM0 to 63
							//     0xC8 (0x08) => Scan from COM63 to 0
	}


	void Set_Display_Offset(unsigned char d)
	{
		Write_Command(0xD3);			// Set Display Offset
		Write_Command(d);			//   Default => 0x00
	}


	void Set_Display_Clock(unsigned char d)
	{
		Write_Command(0xD5);			// Set Display Clock Divide Ratio / Oscillator Frequency
		Write_Command(d);			//   Default => 0x80
							//     D[3:0] => Display Clock Divider
							//     D[7:4] => Oscillator Frequency
	}


	void Set_Power_Save(unsigned char d)
	{
		Write_Command(0xD8);			// Set Low Power Display Mode
		Write_Command(d);			//   Default => 0x00 (Normal Power Display Mode)
	}


	void Set_Precharge_Period(unsigned char d)
	{
		Write_Command(0xD9);			// Set Pre-Charge Period
		Write_Command(d);			//   Default => 0x22 (2 Display Clocks [Phase 2] / 2 Display Clocks [Phase 1])
							//     D[3:0] => Phase 1 Period in 1~15 Display Clocks
							//     D[7:4] => Phase 2 Period in 1~15 Display Clocks
	}


	void Set_Common_Config(unsigned char d)
	{
		Write_Command(0xDA);			// Set COM Pins Hardware Configuration
		Write_Command(0x02|d);			//   Default => 0x12 (0x10)
							//     Alternative COM Pin Configuration
							//     Disable COM Left/Right Re-Map
	}


	void Set_VCOMH(unsigned char d)
	{
		Write_Command(0xDB);			// Set VCOMH Deselect Level
		Write_Command(d);			//   Default => 0x20 (0.77*VCC)
	}


	void Set_NOP()
	{
		Write_Command(0xE3);			// Command for No Operation
	}

	void print_char(char ascii, u8 font, u8 start_page, u8 start_col);

	void OLED_Init(void);
	void Fill_RAM(unsigned char Data);
	char *itoa(signed long number, char *str, int base);
	void reverse(char s[]);

private:
	static const u8 XLevelL=0x00;
	static const u8 XLevelH=0x10;
	static const u16 XLevel=((XLevelH&0x0F)*16+XLevelL);
	static const u8 Max_Column=96;
	static const u8 Max_Row=16;
	static const u8 Brightness=0x3F;
//	static const u8 address = 0x78;

	static const u8 smallfont[241][5]; //defined in the cpp
	//static const u8 largefont[13][9][2]; //same
	static const unsigned char largefont[95][8][2];
	//static const unsigned char largefont[9][8][2];
	static const u8 logo[12][2];
	static const u8 logoname[50][2];

	static const pindef resetpin;
	static const pindef mux[NUM_DISPLAYS];

	u8 mux_select;

	//u8 last_sent_string[3][2][10]; //the last string sent out on the bus, used for comparison purposes
	u8 printbuf[NUM_DISPLAYS][2][96];
	u8 dblbuf[NUM_DISPLAYS][2][96];

	//u16 reset_pin;
	//GPIO_TypeDef * reset_bus;
};












#endif /* PG9616_HPP_ */
