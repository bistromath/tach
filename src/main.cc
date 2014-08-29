/******************** (C) COPYRIGHT 2008 Zylight LLC **************************/


/* Includes ------------------------------------------------------------------*/

extern "C" {
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "misc.h"
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

u8 current_disp = 2;
const float voltage_iir_gain = 0.1;

/* PIN DEFINITIONS */
const pindef stepper::outputs[] = {
            {GPIOA, GPIO_Pin_8},
            {GPIOA, GPIO_Pin_9},
            {GPIOA, GPIO_Pin_10},
            {GPIOA, GPIO_Pin_11}
};

const pindef stepper::enables[] = {
            {GPIOB, GPIO_Pin_12},
            {GPIOB, GPIO_Pin_13},
            {GPIOB, GPIO_Pin_14},
            {GPIOB, GPIO_Pin_15}
};
const pindef pg9616::resetpin = {GPIOB, GPIO_Pin_1};
const pindef adc::adcpins = {GPIOA, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4};
/******************/

int main(void) {
    RCC_Configuration(); //initialize reset and clock control module, including applying power to hardware modules
    NVIC_Configuration(); //intialize interrupts

    systick tick;
    tick.init_hardware();

    //buttons is3buttons;

    delay_us(100); //apparently you gotta let the buttons settle or something.

    pg9616 disp(I2C2); //MUST happen after systick is initialized! uses delay_ms()!

    stepper pointer;

    tach rpm;
    tachobjptr = &rpm;

    adc adc1;
    float temps[7];
    adc1.get_temps(temps);
    float filtered_voltage = temps[4];

    const char degreestring[] = {0x8E, 'C', 0x00};
    volatile uint32_t thisrpm=7000;
    volatile float needlescalar=14750;

    while (1) /* MAIN LOOP WAHOOOOOOOO MAIN LOOP */
    {
        //pointer.seek(rpm.get_rpm() / 20000.0);
        //pointer.seek(i++/14679);
//        thisrpm = rpm.get_rpm();
        pointer.seek(thisrpm/needlescalar);

        adc1.get_temps(temps);
        filtered_voltage = filtered_voltage - voltage_iir_gain*(filtered_voltage-temps[4]);

        switch(current_disp) {
        case 0:
            disp.print(0, int(temps[3] + temps[2]), Font_Large, 3, 0, 0);
            disp.print(0, degreestring, Font_Small, 0, 5);
            disp.print(0, "CHT", Font_Small, 1, 5);
            disp.print(0, int(temps[0]), Font_Large, 2, 0, 5);
            disp.print(0, "OIL", Font_Small, 0, 12);
            disp.print(0, "PSI", Font_Small, 1, 12);
            break;
        case 1:
            disp.print(0, "RPM", Font_Large, 0, 6);
            disp.print(0, thisrpm, Font_Large, 0x05, 0x00, 0x00);
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

/*******************************************************************************
 * Function Name  : RCC_Configuration
 * Description    : Configures the different system clocks.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void RCC_Configuration(void) {
    ErrorStatus HSEStartUpStatus;
    /* RCC system reset(for debug purpose) */
    RCC_DeInit();

    /* Enable HSE */
    RCC_HSEConfig(RCC_HSE_ON); //16MHz xtal

    /* Wait till HSE is ready */

    HSEStartUpStatus = RCC_WaitForHSEStartUp();
    while(RCC_WaitForHSEStartUp() != SUCCESS);

    if (HSEStartUpStatus == SUCCESS) {
        /* Enable Prefetch Buffer */
        FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

        /* Flash 2 wait state */
        FLASH_SetLatency(FLASH_Latency_2);

        /* HCLK = SYSCLK */
        RCC_HCLKConfig(RCC_SYSCLK_Div1);

        /* PCLK2 = HCLK, max 72MHz */
        RCC_PCLK2Config(RCC_HCLK_Div1);

        /* PCLK1 = HCLK/2, max 36MHz */
        RCC_PCLK1Config(RCC_HCLK_Div2);

        /* PLLCLK = 8MHz * 9 = 72 MHz */
        /* We're using a 16MHz clock, so we div by 2 and then mul by 9 to get 72MHz */
//        RCC_PLLConfig(RCC_PLLSource_HSE_Div2, RCC_PLLMul_9);

        /* Enable PLL */
//        RCC_PLLCmd(ENABLE);

        /* Wait till PLL is ready */
//        while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET) {
//        }

        /* Select PLL as system clock source */
//        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

        /* Wait till PLL is used as system clock source */
//        while (RCC_GetSYSCLKSource() != 0x08);
        RCC_SYSCLKConfig(RCC_SYSCLKSource_HSE);
    }
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

/*******************************************************************************
 * Function Name  : NVIC_Configuration
 * Description    : Configures Vector Table base location.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void NVIC_Configuration(void) {
    extern unsigned long _start;
#ifdef  VECT_TAB_RAM
    /* Set the Vector Table base location at 0x20000000 */
    NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0);
#else  /* VECT_TAB_FLASH  */
    /* Set the Vector Table base location at 0x08000000 */
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, u32(&_start) - 0x08000000);
#endif
    /*
        NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
        */
}

void fail(void) { //a catch-all error handler
#ifdef DEBUG
    while (1);
#endif
    SCB->AIRCR = ((u32)0x05FA0000) | (u32)0x04;
}
