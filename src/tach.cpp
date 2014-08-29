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

tach::tach(void) {
	//set up timer 3.
	//how fast does it need to tick? we need to see from 500-12000RPM.
	//therefore, 500RPM should hit around 65000 on the timer.
	//500RPM is 8.33Hz. so the timer should overflow at 8.33Hz.
	//72MHz / (8.33*65536) = divide by 131. we can use a div-by-4 clock,
	//and a prescaler of 32, for a total division of 128. at 500RPM, it will
	//show down to 515Hz. That's probably fine. At 12000RPM, it will show
	//2813 counts. You can sacrifice accuracy at the top end a bit by using a
	//256 divisor, but the downside is it will take twice as long at the low end to read
	//a zero count. with 10 averages, it will take half a second. oh darn.
	//we'll use a prescaler of 64.
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
	tim3init.TIM_Prescaler = 32;
	tim3init.TIM_RepetitionCounter = 0;

	TIM_TimeBaseInit(TIM3, &tim3init);
	TIM_SetCounter(TIM3, 0);

	TIM_ICInitTypeDef tim3icinit;
	tim3icinit.TIM_Channel = TIM_Channel_1;
	tim3icinit.TIM_ICFilter = 0x04;
	tim3icinit.TIM_ICPolarity = TIM_ICPolarity_Falling;
	tim3icinit.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	tim3icinit.TIM_ICSelection = TIM_ICSelection_DirectTI;

    ticksperminute = uint32_t(16.0e6*60 / tim3init.TIM_Prescaler);
    blanking = uint32_t(ticksperminute/18000); //18K max RPM

	TIM_ICInit(TIM3, &tim3icinit);

	TIM_UpdateDisableConfig(TIM3, DISABLE);
	TIM_UpdateRequestConfig(TIM3, TIM_UpdateSource_Regular); //so that only an overflow triggers the interrupt

	//now configure the slave mode controller.
//	TIM_SelectInputTrigger(TIM3, TIM_TS_TI1FP1);
//	TIM_SelectSlaveMode(TIM3, TIM_SlaveMode_Reset);

	NVIC_InitTypeDef nvicinit;
	nvicinit.NVIC_IRQChannel = TIM3_IRQn;
	nvicinit.NVIC_IRQChannelCmd = ENABLE;
	nvicinit.NVIC_IRQChannelPreemptionPriority = 1;
	nvicinit.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvicinit);

	TIM_ITConfig(TIM3, TIM_IT_Update | TIM_IT_CC1, ENABLE);

/*    DMA_InitTypeDef dmainit;
    DMA_StructInit(&dmainit);
    dmainit.DMA_PeripheralBaseAddr = (u32)&TIM3->DMAR;
    dmainit.DMA_MemoryBaseAddr = (u32) this->averages;
    dmainit.DMA_DIR = DMA_DIR_PeripheralSRC;
    dmainit.DMA_BufferSize = tach::numavg;
    dmainit.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dmainit.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dmainit.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    dmainit.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    dmainit.DMA_Mode = DMA_Mode_Circular;
    dmainit.DMA_Priority = DMA_Priority_Medium;
    dmainit.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel6, &dmainit);
    TIM_DMAConfig(TIM3, TIM_DMABase_CCR1, TIM_DMABurstLength_1Transfer);
    TIM_DMACmd(TIM3, TIM_DMA_CC1, ENABLE);
    TIM_SelectCCDMA(TIM3, ENABLE);

    DMA_Cmd(DMA1_Channel6, ENABLE);
*/
    TIM_Cmd(TIM3, ENABLE);
}

void tach::irq_handler(void) {
    static int offset=0;
    //first, determine which one got fired
    if(TIM_GetFlagStatus(TIM3, TIM_FLAG_Update)) {
        //then it's a counter overflow
        averages[offset]=65535;
        offset = (offset+1) % tach::numavg;
    } else {
        uint32_t val = TIM_GetCounter(TIM3);
        if(val > blanking) {
            averages[offset] = val;
            offset = (offset+1) % tach::numavg;
            TIM3->CNT=0;
        }
    }

    TIM_ClearFlag(TIM3, TIM_FLAG_CC1 | TIM_FLAG_Update);
    TIM_ClearITPendingBit(TIM3, TIM_IT_CC1 | TIM_IT_Update);
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
