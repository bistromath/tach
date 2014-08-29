/*
 * adc.cpp
 *
 *  Created on: Feb 12, 2009
 *      Author: bistromath
 */
extern "C" {
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "misc.h"
}

#include "adc.hpp"
#include "types.hpp"
#include <math.h>

const float adc::temp_coeffs[7][2] = {{37.4392, -4.7173}, //oil pressure, [10-180]ohms = [0-80]PSI, 12.6mA, so [0.1260-2.2628]V = [0-80]PSI
								{30.4808, -4.7062}, //oil temp, [0.1544-2.779]V = ????degC??
								{3930.0, 0.018856}, //thermistor, you already know this one, but the math is different -- see get_temps()
								{160.000, -10.0}, //CHT sensor: was 165.537, type J linearized = 0.054423mV/degC, amp gain of 111, that's 6.040953mV/degC
								{5.4444, 0.07}, //Supply voltage
								{1.0, 0.0},
								{1.0, 0.0}};

extern void fail();

void adc::init()
{
	extern adc* adcobjptr;
	adcobjptr = this;
   extern GPIO_InitTypeDef GPIO_InitStructure;

   GPIO_InitStructure.GPIO_Pin = adcpins.pin;
   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
   GPIO_Init(adcpins.port, &GPIO_InitStructure);

   DMA_InitTypeDef dmainit;

   ADC_TypeDef *myadc = ADC1;

   DMA_Channel_TypeDef* mydmachan = DMA1_Channel1;

   dmainit.DMA_PeripheralBaseAddr = (u32)&myadc->DR;
   dmainit.DMA_MemoryBaseAddr = (u32) conversion_array;
   dmainit.DMA_DIR = DMA_DIR_PeripheralSRC;
   dmainit.DMA_BufferSize = num_channels*num_conversions;
   dmainit.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
   dmainit.DMA_MemoryInc = DMA_MemoryInc_Enable;
   dmainit.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
   dmainit.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
   dmainit.DMA_Mode = DMA_Mode_Circular;
   dmainit.DMA_Priority = DMA_Priority_Medium;
   dmainit.DMA_M2M = DMA_M2M_Disable;
   DMA_Init(mydmachan, &dmainit);

   DMA_Cmd(mydmachan, ENABLE);

   ADC_InitTypeDef adcinit;
   RCC_ADCCLKConfig(RCC_PCLK2_Div2);

   adcinit.ADC_Mode = ADC_Mode_Independent;
   adcinit.ADC_ScanConvMode = ENABLE;
   adcinit.ADC_ContinuousConvMode = ENABLE;
   adcinit.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
   adcinit.ADC_DataAlign = ADC_DataAlign_Right;
   adcinit.ADC_NbrOfChannel = num_channels;

   ADC_Init(myadc, &adcinit);

   ADC_RegularChannelConfig(myadc, ADC_Channel_0, 1, ADC_SampleTime_41Cycles5); //Oil pressure
   ADC_RegularChannelConfig(myadc, ADC_Channel_1, 2, ADC_SampleTime_41Cycles5); //Oil temp
   ADC_RegularChannelConfig(myadc, ADC_Channel_2, 3, ADC_SampleTime_41Cycles5); //Cold junction thermistor
   ADC_RegularChannelConfig(myadc, ADC_Channel_3, 4, ADC_SampleTime_41Cycles5); //Cylinder head temp
   ADC_RegularChannelConfig(myadc, ADC_Channel_4, 5, ADC_SampleTime_41Cycles5); //Battery voltage
   ADC_RegularChannelConfig(myadc, ADC_Channel_16, 6, ADC_SampleTime_13Cycles5); //internal temp sensor
   ADC_RegularChannelConfig(myadc, ADC_Channel_17, 7, ADC_SampleTime_41Cycles5); //internal ref sensor

   ADC_Cmd(myadc, ENABLE);
   ADC_TempSensorVrefintCmd(ENABLE);
   //wait for ADC to start up...
   for(int i = 0; i < 0xFFF; i++);
   ADC_ResetCalibration(myadc);
   while(ADC_GetResetCalibrationStatus(myadc));
   ADC_StartCalibration(myadc);
   while(ADC_GetCalibrationStatus(myadc));

   ADC_DMACmd(myadc, ENABLE);
   ADC_Cmd(myadc, ENABLE);
}

void adc::get_temps(float temps[7]) {

	temps[0] = temps[1] = temps[2] = temps[3] = temps[4] = temps[5] = temps[6] = 0;
	for(int i = 0; i < num_conversions; i++) {
		temps[0] += conversion_array[i*num_channels+0];
		temps[1] += conversion_array[i*num_channels+1];
		temps[2] += conversion_array[i*num_channels+2];
		temps[3] += conversion_array[i*num_channels+3];
		temps[4] += conversion_array[i*num_channels+4];
		temps[5] += conversion_array[i*num_channels+5];
		temps[6] += conversion_array[i*num_channels+6];
	}

	temps[6] = (temps[6]) / (num_conversions); //now this contains 1.20V in counts

	temps[0] = (temps[0] * (1.2/temps[6])) / (num_conversions); //get real volts
	temps[1] = (temps[1] * (1.2/temps[6])) / (num_conversions);
	temps[2] = (temps[2] * (1.2/temps[6])) / (num_conversions);
	temps[3] = (temps[3] * (1.2/temps[6])) / (num_conversions);
	temps[4] = (temps[4] * (1.2/temps[6])) / (num_conversions);
	temps[5] = (temps[5] * (1.2/temps[6])) / (num_conversions);

	temps[0] = temps[0] * temp_coeffs[0][0] + temp_coeffs[0][1]; //find value from slope[0] and offset[1]
	temps[1] = temps[1] * temp_coeffs[1][0] + temp_coeffs[1][1];
	//temps[2] = temps[2] * temp_coeffs[2][0] + temp_coeffs[2][1];
	temps[3] = temps[3] * temp_coeffs[3][0] + temp_coeffs[3][1];
	temps[4] = temps[4] * temp_coeffs[4][0] + temp_coeffs[4][1];
	temps[5] = temps[5] * temp_coeffs[5][0] + temp_coeffs[5][1];
	temps[6] = temps[6] * temp_coeffs[6][0] + temp_coeffs[6][1];

	if(temps[0] < 0) temps[0] = 0; //oil pressure shouldn't be negative

	//the thermistor also uses two coefficients, but it's a special case as the math is different.
	float therm_resistance = (temps[2] * 10000) / (3.3 - temps[2]);
	temps[2] = temp_coeffs[2][0] / log(therm_resistance / temp_coeffs[2][1]) - 273.15;
}
