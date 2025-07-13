/*
 * tach.cpp
 *
 *  Created on: Jan 30, 2010
 *      Author: bistromath
 */

extern "C" {
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
}

#include "tach.hpp"

//volatile u16 tach::averages[10];
//TODO FIXME calculate coefficients from APB1 clock rate
//verify APB1 clock rate (should be 32MHz)

tach::tach(void) {
	//set up timer 3.
	//how fast does it need to tick? we need to see from 500-12000RPM.
	//therefore, 500RPM should hit around 65000 on the timer.
	//500RPM is 8.33Hz. so the timer should overflow at 8.33Hz.
	//32MHz / (8.33*65536) = divide by 58. at 500RPM, it will
	//show down to 515Hz. That's probably fine. At 12000RPM, it will show
	//2813 counts. You can sacrifice accuracy at the top end a bit by using a
	//256 divisor, but the downside is it will take twice as long at the low end to read
	//a zero count. with 10 averages, it will take half a second. oh darn.
	//remember to use the overflow ISR to output a "0" value instead of the CC, which will give a bad number on overflow.
    //
    //we need a smarter way to do this.

	GPIO_InitTypeDef gpioinit;

	gpioinit.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	gpioinit.GPIO_Speed = GPIO_Speed_2MHz;
	gpioinit.GPIO_Pin = GPIO_Pin_6;

	GPIO_Init(GPIOA, &gpioinit);

	TIM_TimeBaseInitTypeDef tim3init;
	tim3init.TIM_ClockDivision = TIM_CKD_DIV1;
	tim3init.TIM_CounterMode = TIM_CounterMode_Up;
	tim3init.TIM_Period = 65535;
	tim3init.TIM_Prescaler = 120;
	tim3init.TIM_RepetitionCounter = 0;

	TIM_TimeBaseInit(TIM3, &tim3init);
	TIM_SetCounter(TIM3, 0);

	TIM_ICInitTypeDef tim3icinit;
	tim3icinit.TIM_Channel = TIM_Channel_1;
	tim3icinit.TIM_ICFilter = 0xFF;
	tim3icinit.TIM_ICPolarity = TIM_ICPolarity_Falling;
	tim3icinit.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	tim3icinit.TIM_ICSelection = TIM_ICSelection_DirectTI;

    //x2 because we're wasted spark and using 2 pulses per revolution
    //the 32e6 comes from native clock rate (64e6) / 4 (APB1 prescaler)
    //the +1 on the prescaler comes from god knows where. looks like we
    //miss one edge or something.
    ticksperminute = uint32_t(2*32.0e6*60 / (tim3init.TIM_Prescaler+1));

	TIM_ICInit(TIM3, &tim3icinit);

	TIM_UpdateDisableConfig(TIM3, DISABLE);
	TIM_UpdateRequestConfig(TIM3, TIM_UpdateSource_Regular); //so that only an overflow triggers the interrupt

    //TIM_SelectMasterSlaveMode(TIM3, TIM_MasterSlaveMode_Enable);
    //TIM_ETRConfig(TIM3, TIM_ExtTRGPSC_OFF, TIM_ExtTRGPolarity_NonInverted, 4);
    //TIM_TIxExternalClockConfig(TIM3, TIM_TIxExternalCLK1Source_TI1ED, TIM_ICPolarity_Rising, 4);
    //TIM_SelectSlaveMode(TIM3, TIM_SlaveMode_Reset);
    TIM3->SMCR = 0x05D4; //it's magic!

	NVIC_InitTypeDef nvicinit;
	nvicinit.NVIC_IRQChannel = TIM3_IRQn;
	nvicinit.NVIC_IRQChannelCmd = ENABLE;
	nvicinit.NVIC_IRQChannelPreemptionPriority = 1;
	nvicinit.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvicinit);

    DMA_InitTypeDef dmainit;
    DMA_StructInit(&dmainit);
    dmainit.DMA_PeripheralBaseAddr = (u32)(&TIM3->DMAR);
    dmainit.DMA_MemoryBaseAddr = (u32) &(averages[0]);
    dmainit.DMA_DIR = DMA_DIR_PeripheralSRC;
    dmainit.DMA_BufferSize = tach::numavg;
    dmainit.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dmainit.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dmainit.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    dmainit.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    dmainit.DMA_Mode = DMA_Mode_Circular;
    dmainit.DMA_Priority = DMA_Priority_High;
    dmainit.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel6, &dmainit);
    TIM_DMAConfig(TIM3, TIM_DMABase_CCR1, TIM_DMABurstLength_1Transfer);
    TIM_DMACmd(TIM3, TIM_DMA_CC1, ENABLE);
    TIM_SelectCCDMA(TIM3, ENABLE);

    DMA_Cmd(DMA1_Channel6, ENABLE);

    TIM_Cmd(TIM3, ENABLE);
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
}

//this only fires off when the timer overruns -- in this case,
//we want to fall off to min RPM.
void tach::irq_handler(void) {
    for(int i=0; i<tach::numavg; i++) averages[i] = 65535;
    TIM_ClearFlag(TIM3, TIM_FLAG_Update);
    TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    DMA_Cmd(DMA1_Channel6, ENABLE);
}

uint16_t tach::get_rpm(void) {
    uint64_t sum = 0;
    for(uint16_t i=0; i<tach::numavg; i++) {
        sum += averages[i];
    }
    //sum += TIM_GetCounter(TIM3);

    if(sum == 0) return 0;
    sum /= tach::numavg;

    return (ticksperminute / sum);
}
