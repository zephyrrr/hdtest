#include "intrins.h"
#include "I2C.h"
#include "GPIOConfig.h"
#include "usart2.h"
#include "Delay.h"
//I2C特殊寄存器访问需将P_SW2的bit7置1
u8 tmp[16];
bit INT_flag;		//RTC中断标志位

void RTC_Isr() interrupt 0 using 2
{ 
//		if(INT==0)
//		{			
				INT_flag=1;	
				IE2 = 0x00;		
//		}	
}  


void I2C_Delay(void)
{
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	_nop_();

}


/****************************/
void I2C_Start(void)               //start the I2C, SDA High-to-low when SCL is high
{
	SDA = 1;
	I2C_Delay();
	SCL = 1;
	I2C_Delay();
	SDA = 0;
	I2C_Delay();
	SCL = 0;
	I2C_Delay();
}		


void I2C_Stop(void)					//STOP the I2C, SDA Low-to-high when SCL is high
{
	SDA = 0;
	I2C_Delay();
	SCL = 1;
	I2C_Delay();
	SDA = 1;
	I2C_Delay();
}

void S_ACK(void)              //Send ACK (LOW)
{
	SDA = 0;
	I2C_Delay();
	SCL = 1;
	I2C_Delay();
	SCL = 0;
	I2C_Delay();
}

void S_NoACK(void)           //Send No ACK (High)
{
	SDA = 1;
	I2C_Delay();
	SCL = 1;
	I2C_Delay();
	SCL = 0;
	I2C_Delay();
}
		
void I2C_Check_ACK(void)         //Check ACK, If F0=0, then right, if F0=1, then error
{
	SDA = 1;
	I2C_Delay();
	SCL = 1;
	I2C_Delay();
	F0  = SDA;
	SCL = 0;
	I2C_Delay();
}

/****************************/
void I2C_WriteAbyte(u8 dat)		//write a byte to I2C
{
	u8 i;
	i = 8;
	do
	{
		if(dat & 0x80)	SDA = 1;
		else			SDA	= 0;
		dat <<= 1;
		I2C_Delay();
		SCL = 1;
		I2C_Delay();
		SCL = 0;
		I2C_Delay();
	}
	while(--i);
}

/****************************/
u8 I2C_ReadAbyte(void)			//read A byte from I2C
{
	u8 i,dat;
	i = 8;
	SDA = 1;
	do
	{
		SCL = 1;
		I2C_Delay();
		dat <<= 1;
		if(SDA)		dat++;
		SCL  = 0;
		I2C_Delay();
	}
	while(--i);
	return(dat);
}

/****************************/
void WriteNbyte(u8 addr, u8 *p, u8 number)			/*	WordAddress,First Data Address,Byte lenth	*/
                         									//F0=0,right, F0=1,error
{
	I2C_Start();
	I2C_WriteAbyte(SLAW);
	I2C_Check_ACK();
	if(!F0)
	{
		I2C_WriteAbyte(addr);
		I2C_Check_ACK();
		if(!F0)
		{
			do
			{
				I2C_WriteAbyte(*p);		p++;
				I2C_Check_ACK();
				if(F0)	break;
			}
			while(--number);
		}
	}
	I2C_Stop();
}


/****************************/
void ReadNbyte(u8 addr, u8 *p, u8 number)		/*	WordAddress,First Data Address,Byte lenth	*/
{
	I2C_Start();
	I2C_WriteAbyte(SLAW);
	I2C_Check_ACK();
	if(!F0)
	{
		I2C_WriteAbyte(addr);
		I2C_Check_ACK();
		if(!F0)
		{
			I2C_Start();
			I2C_WriteAbyte(SLAR);
			I2C_Check_ACK();
			if(!F0)
			{
				do
				{
					*p = I2C_ReadAbyte();	p++;
					if(number != 1)		S_ACK();	//send ACK
				}
				while(--number);
				S_NoACK();			//send no ACK
			}
		}
	}
	I2C_Stop();
}



void RTC_init()
{
	tmp[0]=0x00;		//控制/状态寄存器1设置，正常模式，STOP=0，芯片时钟运行，加电复位失效功能禁止
	tmp[1]=0x02;		//控制/状态寄存器2设置，TI/TP=0，AIE=0，TIE=1
	tmp[2]=0x00;		//秒寄存器，设置为0，当前秒数0，可保证准确的时钟/日历数据
	tmp[3]=0x19;		//分寄存器，设置为0，当前分钟数0,
	tmp[4]=0x23;		//时寄存器，设置为0，当前小时数0
	tmp[5]=0x29;		//天寄存器，当前天数01
	tmp[6]=0x04;		//星期寄存器，设置为0，当前周日
	tmp[7]=0x07;		//月寄存器，设置为7，当前7月，21世纪
	tmp[8]=0x21;		//年寄存器，设置为21，当前年份21
	//tmp[9]=0x59;		//分报警寄存器，使能分钟报警，到下一个第59分钟报警  //0x01分报警     //0x80不报警
	tmp[9]=0x80;
	tmp[10]=0x80;		//时报警寄存器，禁止小时报警
	tmp[11]=0x80;		//天报警寄存器，禁止日期报警
	tmp[12]=0x80;		//周报警寄存器，禁止周报警
	tmp[13]=0x00;		//设置CLKOUT引脚为高阻
	//tmp[14]=0x82;		//设置定时器控制器寄存器TE=1,TD1=1,TD0=0(1Hz）
	//tmp[15]=0x3c;		//设置定时器到计数器数值为0x3c，即60s
	tmp[14]=0x03;		//设置定时器控制器寄存器TE=1,TD1=1,TD0=1(1/60Hz）
	tmp[15]=0x00;		//设置定时器到计数器数值为0xf0，即14400s
	WriteNbyte(0x00, tmp, 16);			//初始化pcf8563
	IP=0x01;
	IPH=0x01;				//RTC优先级为3级(最高级)
}

