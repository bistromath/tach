/******************** (C) COPYRIGHT 2008 Zylight LLC **************************/


/* Includes ------------------------------------------------------------------*/

extern "C" {
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "misc.h"
#include "math.h"
#include <stdlib.h>
}

#include "pg9616.hpp"
#include "systick.hpp"
#include "tick_callback.hpp"
#include "types.hpp"
#include <sys/types.h>
#include "syscalls.h"
#include "circular_buffer.hpp"
#include "adc.hpp"
#include "serial.hpp"
#include "tach.hpp"
#include "stepper.hpp"
extern "C" {
#include <stdio.h>
}

static const u8 version_major = VERSION_MAJOR;
static const u8 version_minor = VERSION_MINOR; //defined in types.hpp so that usblib has access, too

//GLOBAL POINTERS: these are generally used for interrupt access to program data
i2c_device* i2cobjptr; //global pointer to I2C object, used as a virtualizer to call I2C interrupt handler
adc* adcobjptr; //so the interrupt can handle particular data
serial* serialobjptr;
tach* tachobjptr;

/* FLASH variables, stored in the last page of ROM */


/* END FLASH RESIDENT VARS */

GPIO_InitTypeDef GPIO_InitStructure; //for clarity, since its contents never need to be saved

/* Private function prototypes -----------------------------------------------*/
void delay_ms(u16 delay);
void delay_us(u32 delay);

//who knows where this goes
extern "C" void debug_write(char *str, int len);

void RCC_Configuration();
void NVIC_Configuration();

void fail(void);

//which info screen is currently active
u8 current_disp = 0;

//filter constants
const float voltage_iir_gain = 0.1;
const float temp_iir_gain = 0.02;
const float rpm_iir_gain = 0.1;

//tach calibration
const uint32_t needlescalar=14800;

/* PIN DEFINITIONS */
const pindef stepper::outputs[] = {
            {GPIOA, GPIO_Pin_11},
            {GPIOA, GPIO_Pin_10},
            {GPIOA, GPIO_Pin_9},
            {GPIOA, GPIO_Pin_8}
};

const pindef stepper::enables[] = {
            {GPIOB, GPIO_Pin_12},
};

const pindef pg9616::resetpin = {GPIOB, GPIO_Pin_2};
const pindef adc::adcpins = {GPIOA, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3};


/* voltage conversion functions
 * these convert the sensor voltages to outputs.
*/
/*******************************/
float conv_batt_volt(float voltage) {
    return 4.9705*voltage;
}

//christ can you imagine how long this takes to run
//this outputs deg C
float conv_cold_junc_temp(float voltage) {
    return -1481.96+sqrt(2.1962e6+((1.8639-(voltage-0.00))/3.88e-6));
}

//this outputs deg F, since it's used directly for display
float conv_oil_temp(float voltage) {
    return 144.796*pow(voltage, -0.3163449);
}

//note: this does not compensate for cold junction temp.
//the output of this is deg C.
//50uV/deg C
//gain of thermocouple amplifier is 151
//so, 7550uV/deg C
//this gets us up to ~400C before we saturate
//we could probably increase the gain some, if we
//wanted better resolution.
float conv_cht(float voltage) {
    return voltage/0.00755;
}
/*******************************/


int main(void) {
    RCC_Configuration(); //initialize reset and clock control module, including applying power to hardware modules
    //NVIC_Configuration(); //intialize interrupts

    systick tick;
    tick.init_hardware();


    stepper pointer;

    tach rpm;
    tachobjptr = &rpm;

    adc adc1;
    adc1.reg_converter(0, &conv_batt_volt);
    adc1.reg_converter(1, &conv_oil_temp);
    adc1.reg_converter(2, &conv_cht);
    adc1.reg_converter(3, &conv_cold_junc_temp);

    systick disp_timer;
    disp_timer.start();

    const char degreestring[] = {0x8E, 'F', 0x00};
    uint32_t thisrpm=0;
    float filtered_rpm=0;
    float filtered_voltage=adc1.get_value(0);
    float filtered_oil_temp=adc1.get_value(1);
    float filtered_coldjunc_temp=adc1.get_value(3);
    float filtered_cht=adc1.get_value(2);
    float cht;

    pg9616 disp(I2C2); //MUST happen after systick is initialized! uses delay_ms()!

    while (1) /* MAIN LOOP WAHOOOOOOOO MAIN LOOP */
    {
        thisrpm = rpm.get_rpm();
        if(thisrpm < 350) thisrpm = 0;

        //moving avg filter for sensors
        filtered_rpm = filtered_rpm - rpm_iir_gain*(filtered_rpm-thisrpm);
        filtered_voltage = filtered_voltage - voltage_iir_gain*(filtered_voltage-adc1.get_value(0));
        filtered_oil_temp = filtered_oil_temp - temp_iir_gain*(filtered_oil_temp-adc1.get_value(1));
        filtered_coldjunc_temp = filtered_coldjunc_temp - temp_iir_gain*(filtered_coldjunc_temp-adc1.get_value(3));
        filtered_cht = filtered_cht - temp_iir_gain*(filtered_cht-adc1.get_value(2));
        cht = (filtered_cht+filtered_coldjunc_temp)*9/5 + 32;

        pointer.seek(filtered_rpm*stepper::max_position/needlescalar);

        switch(current_disp) {
        case 0:
            if(filtered_oil_temp < 120) {
                disp.print(0, "---", Font_Large, 0, 0, 0);
            } else {
                disp.print(0, round(filtered_oil_temp), Font_Large, 3, 0, 0);
            }
            disp.print(0, degreestring, Font_Small, 0, 5, -1);
            disp.print(0, "OIL", Font_Small, 1, 5, 0);
            disp.print(0, round(cht), Font_Large, 3, 0, 5);
            disp.print(0, degreestring, Font_Small, 0, 13, 1);
            disp.print(0, "CHT", Font_Small, 1, 13, 2);
            break;
        case 1:
            disp.print(0, "RPM", Font_Large, 0, 6);
            disp.print(0, round(filtered_rpm), Font_Large, 0x05, 0x00, 0x00);
            break;
        case 2:
            disp.print(0, filtered_voltage, Font_Large, 5, 0, 2, false, 2);
            disp.print(0, "V", Font_Large, 0, 7);
            break;
        default:
            current_disp = 0;
            break;
        }

        disp.update(); //update all displays at the end of each cycle
        delay_ms(10);

        if(disp_timer.get_elapsed_ms() > 2000) {
            disp_timer.stop();
            disp_timer.reset();
            disp_timer.start();
            disp.clear();
            current_disp = current_disp==0 ? 2 : 0;
            //thisrpm = thisrpm + 1000;
            //if(thisrpm > 12000) thisrpm = 0;
        }
    }

    //EXECUTION WILL NEVER REACH THIS POINT
    fail();
}

void delay_ms(u16 delay) {
	delay_us((u32) (delay * 1000));
}

//find the delay in this function and calibrate it by decreasing the delay value (done, 7us)
//looks like this is pretty damn accurate when not interrupted
//unlike a fixed spin delay, if time is taken during an interrupt it won't throw this calc off
//unless it's in an interrupt when the time delay is up.
void delay_us(u32 delay) {
	systick t;
	t.start();
	if (delay > 7)
		while (t.get_elapsed_us() < (u32) (delay - 7))
			;
	t.dealloc();
}

extern "C" void debug_write(char* str, int len) {
	serialobjptr->send(str, len);
}

void RCC_Configuration(void) {
    /* Enable peripheral clocks */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
//  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
//  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
//  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
//  RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);

    //RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
//  RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
//  RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
//  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);
    /* Enable USB clock */
//  RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);
//  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
    //RCC_APB1PeriphClockCmd(RCC_APB1Periph_ALL, ENABLE); //just to be sure
    //RCC_APB2PeriphClockCmd(RCC_APB2Periph_ALL, ENABLE); //just to be sure
    //RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, DISABLE); //gimme that pin back!
    //RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, DISABLE);
    //RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN, DISABLE);
}

void fail(void) { //a catch-all error handler
#ifdef DEBUG
    while (1);
#endif
    SCB->AIRCR = ((u32)0x05FA0000) | (u32)0x04;
}
