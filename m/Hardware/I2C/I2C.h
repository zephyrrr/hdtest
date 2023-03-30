#ifndef __I2C_H_
#define __I2C_H_

#include  "STC8.h"

#define SLAW	0xA2		//写地址
#define SLAR	0xA3		//读地址

extern bit INT_flag;
#define		SDA	  P24
#define		SCL	  P25	
#define 	INT		P32



void	I2C_Delay(void);
void	I2C_Start(void);               //start the I2C, SDA High-to-low when SCL is high
void	I2C_Stop(void);								 //STOP the I2C, SDA Low-to-high when SCL is high
void	S_ACK(void);              		 //Send ACK (LOW)
void  S_NOACK(void);
void	I2C_Check_ACK();         	 //Check ACK, If F0=0, then right, if F0=1, then error
void	I2C_WriteAbyte(u8 dat);				 //write a byte to I2C
u8		I2C_ReadAbyte(void);		 			 //read A byte from I2C
void	WriteNbyte(u8 addr, u8 *p, u8 number);			/*	WordAddress,First Data Address,Byte lenth	*/
void	ReadNbyte(u8 addr, u8 *p, u8 number);		/*	WordAddress,First Data Address,Byte lenth	*/
void	RTC_init();         					 //实现RTC定时器倒计数定时1s相关寄存器的初始化


#endif