/*
 * i2c_object.cc
 *
 *  Created on: Oct 8, 2008
 *      Author: bistromath
 */
extern "C" {
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "misc.h"
}

#include "i2c.hpp"

void i2c_device::init(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	GPIO_InitTypeDef gpioinit;
	gpioinit.GPIO_Speed = GPIO_Speed_50MHz;
	gpioinit.GPIO_Mode = GPIO_Mode_AF_OD; //alt function, open drain
	//this function must be overloaded by children since the specifics of each initialization are different
	//this currently does not support remapping; will have to change the constructor to take an additional argument if you want it
	switch((u32)i2cdev) {
	case I2C1_BASE:
		gpioinit.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
		GPIO_Init(GPIOB, &gpioinit);
		NVIC_InitStructure.NVIC_IRQChannel = I2C2_EV_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
		NVIC_InitStructure.NVIC_IRQChannel = I2C2_ER_IRQn;
		NVIC_Init(&NVIC_InitStructure);
		break;
	case I2C2_BASE:
		gpioinit.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
		GPIO_Init(GPIOB, &gpioinit);
		NVIC_InitStructure.NVIC_IRQChannel = I2C2_EV_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
		NVIC_InitStructure.NVIC_IRQChannel = I2C2_ER_IRQn;
		NVIC_Init(&NVIC_InitStructure);
		break;
	default:
		break;
	}
}

void i2c_device::deinit(void)
{
	I2C_DeInit(i2cdev);
}

void i2c_device::send(u8 len)
{
	extern i2c_device* i2cobjptr;
	txlen = len;
	if(txlen > TXBUFLEN) return; //should throw an exception
   	dir = I2C_TRANSMIT;
  	tx_idx = 0;
    rx_idx = 0;
    /* Enable I2C acknowledgement because it was disabled after I2C received bytes from Slave */
    I2C_AcknowledgeConfig(i2cdev, ENABLE);
    i2cobjptr = this; //set the i2c interrupt handler vector to this instance
    busyflag = 1;
    I2C_ITConfig(i2cdev, I2C_IT_EVT | I2C_IT_BUF, ENABLE);
    /* Send START condition */
    I2C_GenerateSTART(i2cdev, ENABLE); //after this start, the I2C interrupt takes care of the rest
}

void i2c_device::txnext(void) {
	if(tx_idx < txlen) {
		//I2C_ClearFlag(i2cdev, I2C_FLAG_TXE); //no, if this irq fires it's because txe is asserted

		I2C_SendData(i2cdev, txbuf[tx_idx++]);
	}
	else {
		I2C_ITConfig(i2cdev, I2C_IT_BUF | I2C_IT_EVT, DISABLE); //welp that was it
		//dir = I2C_RECEIVE;  /* Change data direction */
		//delay_us(100);
		//I2C_ClearFlag(i2cdev, I2C_FLAG_BTF);
		//while(I2C_GetFlagStatus(i2cdev, I2C_FLAG_BTF));
		//while(!(I2C_GetFlagStatus(i2cdev, I2C_FLAG_TXE) || I2C_GetFlagStatus(i2cdev, I2C_FLAG_BTF)));
		I2C_GenerateSTOP(i2cdev, ENABLE); /* Re-Start (was generateSTART)*/

		//delay_us(10);
		busyflag = 0;
	}
}

//void i2c_device::send(u8 *buf, u8 len)
//{
//	txbufptr = buf;
//	send(len);
//}

#define I2C_EVENT_MASTER_UNK_FAILURE ((u32)0x00070004)

void i2c_device::irqhandler() {
	extern void delay_us(u32);
    switch (I2C_GetLastEvent(i2cdev))
    {
          case I2C_EVENT_MASTER_MODE_SELECT:        /* EV5, send address query */
              if (dir == I2C_RECEIVE)
              {
                  /* Send slave Address for read -- replace I2C2_SLAVE_ADDRESS7 with the address of the LCD panel */
                  I2C_Send7bitAddress(i2cdev, address, I2C_Direction_Receiver);
              }
              else
              {
                  /* Send slave Address for write -- same as above */
                  I2C_Send7bitAddress(i2cdev, address, I2C_Direction_Transmitter);
              }

              break;
              /* Master Receiver events */
          case I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED:  /* EV6, set up for Master Rx mode */
              if (RXBUFLEN == 1) //wat
              {
                  /* Disable I2C1 acknowledgement */
                  I2C_AcknowledgeConfig(i2cdev, DISABLE);
                  /* Send I2C1 STOP Condition */
                  I2C_GenerateSTOP(i2cdev, ENABLE);
              }
              break;

          case I2C_EVENT_MASTER_BYTE_RECEIVED:    /* EV7, master receiver rx byte */
              /* Store I2C1 received data */
        	  rxnext();
              /* Disable ACK and send I2C1 STOP condition before receiving the last data */
              if (rx_idx == rxlen-1)
              {
                  /* Disable I2C1 acknowledgement */
                  I2C_AcknowledgeConfig(i2cdev, DISABLE);
                  /* Send I2C1 STOP Condition */
                  I2C_GenerateSTOP(i2cdev, ENABLE);
              }
              break;

              /* Master Transmitter events */
          case I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED: /* EV8-1 just after EV6 */
        	  //delay_us(10);
        	  txnext();
              break;

          case I2C_EVENT_MASTER_BYTE_TRANSMITTING:       /* EV8 */
        	  //delay_us(10);
        	  txnext();
              break;

          case I2C_EVENT_MASTER_BYTE_TRANSMITTED:       /* EV8-2 */
              //I2C_ITConfig(i2cdev, I2C_IT_BUF | I2C_IT_EVT, DISABLE); //welp that was it
              //dir = I2C_RECEIVE;  /* Change data direction */
              //delay_us(100);
              //I2C_GenerateSTOP(i2cdev, ENABLE); /* Re-Start (was generateSTART)*/
              //delay_us(10);
              //busyflag = 0;
        	  txnext();
              break;

          case I2C_EVENT_MASTER_UNK_FAILURE: //this is the weird bus condition you're seeing
        	  I2C_ITConfig(i2cdev, I2C_IT_BUF | I2C_IT_EVT, DISABLE); //this will just cancel the transmission
        	  I2C_GenerateSTOP(i2cdev, ENABLE); //and wait for the next one to fix everything. you should really
        	  busyflag = 0; //raise an error flag or something to ensure an uncorrupted display.

          default: //wat happened aaahhhh
        	  I2C_ITConfig(i2cdev, I2C_IT_BUF | I2C_IT_EVT, DISABLE); //this will just cancel the transmission
        	  I2C_GenerateSTOP(i2cdev, ENABLE); //and wait for the next one to fix everything. you should really
        	  busyflag = 0; //raise an error flag or something to ensure an uncorrupted display.
              break;
    }

}

void i2c_device::errorhandler(void)
{
	//here we handle errors! hooray.
	I2C_ITConfig(i2cdev, I2C_IT_BUF | I2C_IT_EVT, DISABLE); //welp that was it
	I2C_GenerateSTOP(i2cdev, ENABLE); /* Re-Start (was generateSTART)*/
	busyflag = 0;
}








