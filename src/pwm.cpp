/*
 * pwm.cpp
 *
 *  Created on: Oct 16, 2008
 *      Author: bistromath
 */

extern "C" {
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
}

#include "pwm.hpp"

void pwm::init_timer(void) {
	TIM_TimeBaseInitTypeDef tim1init;
	TIM_OCInitTypeDef tim1pwminit;

	tim1init.TIM_ClockDivision = TIM_CKD_DIV1;
	tim1init.TIM_CounterMode = TIM_CounterMode_CenterAligned2;
	tim1init.TIM_Period = 1000;
	tim1init.TIM_Prescaler = 1;
	tim1init.TIM_RepetitionCounter = 0;

	//below line is for enable-line PWM
#ifndef USE_OUTPUT_FETS
	tim1pwminit.TIM_OCMode = TIM_OCMode_PWM2; //necessary for non-inverted center-aligned pwm
	tim1pwminit.TIM_OCIdleState = TIM_OCIdleState_Set; //i guess this defines the output pin value when counter is idle?
	tim1pwminit.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
	tim1pwminit.TIM_OCPolarity = TIM_OCPolarity_High; //necessary for non-inverted center-aligned pwm
	tim1pwminit.TIM_OCNPolarity = TIM_OCNPolarity_Low; //polarity non-inverted (shouldn't matter when disabled)
#else
	tim1pwminit.TIM_OCMode = TIM_OCMode_PWM2;
	tim1pwminit.TIM_OCIdleState = TIM_OCIdleState_Reset; //i guess this defines the output pin value when counter is idle?
	tim1pwminit.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
	tim1pwminit.TIM_OCPolarity = TIM_OCPolarity_Low; //necessary for non-inverted center-aligned pwm
	tim1pwminit.TIM_OCNPolarity = TIM_OCNPolarity_Low; //polarity non-inverted (shouldn't matter when disabled)
#endif

	tim1pwminit.TIM_OutputState = TIM_OutputState_Enable; //output enable
	tim1pwminit.TIM_OutputNState = TIM_OutputNState_Disable;
	tim1pwminit.TIM_Pulse = TIM_OPMode_Repetitive; //one pulse mode
	TIM_TimeBaseInit(timer, &tim1init);
	TIM_OC1Init(timer, &tim1pwminit);
	TIM_OC2Init(timer, &tim1pwminit);
	TIM_OC3Init(timer, &tim1pwminit);
	TIM_OC4Init(timer, &tim1pwminit);
	TIM_SetCompare1(timer, 0);
	TIM_SetCompare2(timer, 0);
	TIM_SetCompare3(timer, 0);
	TIM_SetCompare4(timer, 0);
	TIM_CtrlPWMOutputs(timer, ENABLE);
	TIM_UpdateDisableConfig(timer, DISABLE);//hmmmmm
	TIM_CCxCmd(timer, TIM_Channel_1, ENABLE);
	TIM_CCxCmd(timer, TIM_Channel_2, ENABLE);
	TIM_CCxCmd(timer, TIM_Channel_3, ENABLE);
	TIM_CCxCmd(timer, TIM_Channel_4, ENABLE);

	TIM_CCPreloadControl(timer, DISABLE);
	TIM_OC1PreloadConfig(timer, DISABLE);
	TIM_OC2PreloadConfig(timer, DISABLE);
	TIM_OC3PreloadConfig(timer, DISABLE);
	TIM_OC4PreloadConfig(timer, DISABLE);
//	TIM_OC1FastConfig(timer, ENABLE);
//	TIM_OC2FastConfig(timer, ENABLE);
//	TIM_OC3FastConfig(timer, ENABLE);
//	TIM_OC4FastConfig(timer, ENABLE);
	//TIM_ITConfig(TIM1, TIM_IT_CC4 | TIM_IT_Update, ENABLE);
	clearlimit();
	setraw(0,0);
	setraw(1,0);
	setraw(2,0);
	setraw(3,0);
	TIM_Cmd(timer, ENABLE);
	enable();
}

void pwm::init_outputs() {
	GPIO_InitTypeDef gpioinit;

	if(timer == TIM1) { //full remap
		GPIO_PinRemapConfig(GPIO_FullRemap_TIM1, ENABLE);
		gpioinit.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_11 | GPIO_Pin_13 | GPIO_Pin_14;
		gpioinit.GPIO_Speed = GPIO_Speed_50MHz;
		gpioinit.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_Init(GPIOE, &gpioinit);
	}
	if(timer == TIM3) { //for now
		gpioinit.GPIO_Pin = GPIO_Pin_0;
		gpioinit.GPIO_Speed = GPIO_Speed_50MHz;
		gpioinit.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_Init(GPIOB, &gpioinit);
	}
	if(timer == TIM5) { //we're just using timer 5's CH3 output for the diffuser output
		gpioinit.GPIO_Pin = GPIO_Pin_2;
		gpioinit.GPIO_Speed = GPIO_Speed_50MHz;
		gpioinit.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_Init(GPIOA, &gpioinit);
	}
	if(timer == TIM8) { //except we're using timer 8 instead
		//gpioinit.GPIO_Pin =
	}

#ifdef USE_OUTPUT_FETS
	gpioinit.GPIO_Pin = GPIO_Pin_4;
	gpioinit.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOE, &gpioinit);
#endif
}

void pwm::setlimit() {
	//limit = 1;
//	GPIO_InitTypeDef gpioinit;
//	gpioinit.GPIO_Mode = GPIO_Mode_IPD;
//	gpioinit.GPIO_Pin = GPIO_Pin_14;
//	GPIO_Init(GPIOE, &gpioinit);
//	TIM_CtrlPWMOutputs(timer, DISABLE);
#ifndef USE_OUTPUT_FETS
	TIM_CCxCmd(timer, TIM_Channel_1, DISABLE);
	TIM_CCxCmd(timer, TIM_Channel_2, DISABLE);
	TIM_CCxCmd(timer, TIM_Channel_3, DISABLE);
	TIM_CCxCmd(timer, TIM_Channel_4, DISABLE);
#else
	GPIO_ResetBits(GPIOE, GPIO_Pin_4);
#endif
}

void pwm::clearlimit() {
//	TIM_CtrlPWMOutputs(timer, ENABLE);
//	GPIO_InitTypeDef gpioinit;
//	gpioinit.GPIO_Mode = GPIO_Mode_AF_PP;
//	gpioinit.GPIO_Pin = GPIO_Pin_14;
//	GPIO_Init(GPIOE, &gpioinit);
#ifndef USE_OUTPUT_FETS
	TIM_CCxCmd(timer, TIM_Channel_1, ENABLE);
	TIM_CCxCmd(timer, TIM_Channel_2, ENABLE);
	TIM_CCxCmd(timer, TIM_Channel_3, ENABLE);
	TIM_CCxCmd(timer, TIM_Channel_4, ENABLE);
#else
	GPIO_SetBits(GPIOE, GPIO_Pin_4);
#endif
}

void pwm::disable() {
#ifdef USE_OUTPUT_FETS
	GPIO_ResetBits(GPIOE, GPIO_Pin_4);
#endif
}

void pwm::enable() {
#ifdef USE_OUTPUT_FETS
	GPIO_SetBits(GPIOE, GPIO_Pin_4);
#endif
}
