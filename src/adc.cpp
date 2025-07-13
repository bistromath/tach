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

//this is nominally 1.2V for the STM32F
//internal reference. unfortunately it is not
//very accurate, and should be calibrated.
const float ADC_VREF = 1.205;
const float EXT_VREF = 2.500;

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

   ADC_InitTypeDef adcinit;
   RCC_ADCCLKConfig(RCC_PCLK2_Div4);

   adcinit.ADC_Mode = ADC_Mode_Independent;
   adcinit.ADC_ScanConvMode = ENABLE;
   adcinit.ADC_ContinuousConvMode = ENABLE;
   adcinit.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
   adcinit.ADC_DataAlign = ADC_DataAlign_Right;
   adcinit.ADC_NbrOfChannel = num_channels;

   ADC_Init(myadc, &adcinit);

   ADC_RegularChannelConfig(myadc, ADC_Channel_0, 1, ADC_SampleTime_41Cycles5); //Battery voltage
   ADC_RegularChannelConfig(myadc, ADC_Channel_1, 2, ADC_SampleTime_41Cycles5); //Oil temp
   ADC_RegularChannelConfig(myadc, ADC_Channel_2, 3, ADC_SampleTime_41Cycles5); //Thermocouple
   ADC_RegularChannelConfig(myadc, ADC_Channel_3, 4, ADC_SampleTime_41Cycles5); //Cold junction
   ADC_RegularChannelConfig(myadc, ADC_Channel_4, 5, ADC_SampleTime_41Cycles5); //external vref
   ADC_RegularChannelConfig(myadc, ADC_Channel_TempSensor, 6, ADC_SampleTime_41Cycles5); //internal temp sensor
   ADC_RegularChannelConfig(myadc, ADC_Channel_Vrefint, 7, ADC_SampleTime_41Cycles5); //internal ref sensor


   ADC_Cmd(myadc, ENABLE);
   ADC_TempSensorVrefintCmd(ENABLE);
   //wait for ADC to start up...
   for(int i = 0; i < 0xFFF; i++);

   ADC_ResetCalibration(myadc);
   while(ADC_GetResetCalibrationStatus(myadc));
   ADC_StartCalibration(myadc);
   while(ADC_GetCalibrationStatus(myadc));

   for(int i=0; i<num_channels; i++) converters[i] = NULL; //FIXME your bss is not working
   ADC_Cmd(myadc, ENABLE);
   ADC_DMACmd(myadc, ENABLE);
   DMA_Cmd(mydmachan, ENABLE);
   for(int i=0; i<(45*num_channels*num_conversions); i++); //let us convert a full buffer
}

void adc::reg_converter(u8 channel, adc::convert_t converter) {
    converters[channel] = converter;
}

float adc::get_voltage(u8 channel) {
//a little wasteful to do the ref calcs each time
    uint32_t sum=0, refsum=0;
    for(int i=0; i<num_conversions; i++) {
        sum += conversion_array[i*num_channels+channel];
        refsum += conversion_array[i*num_channels+4]; //external ref
    }
    uint16_t avg = sum / num_conversions; //close enough
    uint16_t refavg = refsum / num_conversions; //now contains ADC_VREF in counts

    float voltage = avg*EXT_VREF/refavg;
    return voltage;
}

float adc::get_value(u8 channel) {
    float voltage = get_voltage(channel);
    if(converters[channel]) return converters[channel](voltage);
    else return 0;
}
