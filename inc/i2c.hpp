//i2c.hpp
//I2C class definition for use by I2C devices such as Univision OLED display, EEPROM, etc.
//this class implements an I2C MASTER. if you want a slave, do it yourself.
//hey also uh you haven't tested i2c rx yet, so eventually you should do that
//shortened buffer sizes, make sure that's ok

//TODO: DMA support to remove CPU dependence?
//Circular buffers (supported by DMA)?

extern "C" {
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
}

#include "types.hpp"

#define I2C_TRANSMIT 0
#define I2C_RECEIVE 1
#define TXBUFLEN 138
#define RXBUFLEN 64

class i2c_device
{
    public:
        //constructor given I2C address
        i2c_device(I2C_TypeDef* I2Cx, u8 addr) : i2cdev(I2Cx), address(addr), tx_idx(0), rx_idx(0), drdy(0), busyflag(0) { }

        void init(void);
        void deinit(void);

        void send(u8 len); //sends data
        //void send(u8 *buf, u8 len); //sends data from *buf
        void readreg(u8 reg, u8 len); //read register
//        void settxbuf(u8 *data, u8 len) {memcpy(txbufptr, data, len);} //uses memcpy to put data into the tx buffer
//        void setrxbuf(u8 *data, u8 len) {memcpy(rxbufptr, data, len);} //wat

//        u8 *gettxbuf(void) const { return txbufptr; } //accessor functions
//        u8 *getrxbuf(void) const { return rxbufptr; }
        u8 getaddress(void) const { return address; }
        u8 getrxbufsize(void) const { return RXBUFLEN; }
        u8 gettxbufsize(void) const { return TXBUFLEN; }
        void dataready(bool datardy) { drdy = datardy; }
        bool dataready(void) const { return drdy; }
        bool direction(void) const { return dir; }
        void direction(bool direc) { dir = direc; }
        u8 gettxindex(void) const { return tx_idx; }
        u8 getrxindex(void) const { return rx_idx; }

        void rxnext(void) {
							rxbuf[rx_idx++] = I2C_ReceiveData(i2cdev);
							if(rx_idx == RXBUFLEN)
							{
								drdy = 1;
								busyflag = 0;
							}
						  }
        void txnext(void);

        void irqhandler(void);
        void errorhandler(void);
        bool busy(void) { return busyflag | I2C_GetFlagStatus(i2cdev, I2C_FLAG_BUSY); } //i don't know if this works, it might not be interrupt-safe

    protected:
        I2C_TypeDef *i2cdev;
        u8 address;
        volatile u8 txbuf[TXBUFLEN];
        volatile u8 rxbuf[RXBUFLEN];
        //u8 rxbuflen, txbuflen;
        volatile u8 tx_idx, rx_idx;
        volatile bool drdy;
        volatile bool dir;
        volatile bool busyflag;
        u8 txlen, rxlen;

    private:
};
