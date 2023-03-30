#include "intrins.h"
#include "I2C.h"
#include "GPIOConfig.h"
#include "usart2.h"
#include "Delay.h"
//I2C����Ĵ��������轫P_SW2��bit7��1
u8 tmp[16];
bit INT_flag;		//RTC�жϱ�־λ

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
	tmp[0]=0x00;		//����/״̬�Ĵ���1���ã�����ģʽ��STOP=0��оƬʱ�����У��ӵ縴λʧЧ���ܽ�ֹ
	tmp[1]=0x02;		//����/״̬�Ĵ���2���ã�TI/TP=0��AIE=0��TIE=1
	tmp[2]=0x00;		//��Ĵ���������Ϊ0����ǰ����0���ɱ�֤׼ȷ��ʱ��/��������
	tmp[3]=0x19;		//�ּĴ���������Ϊ0����ǰ������0,
	tmp[4]=0x23;		//ʱ�Ĵ���������Ϊ0����ǰСʱ��0
	tmp[5]=0x29;		//��Ĵ�������ǰ����01
	tmp[6]=0x04;		//���ڼĴ���������Ϊ0����ǰ����
	tmp[7]=0x07;		//�¼Ĵ���������Ϊ7����ǰ7�£�21����
	tmp[8]=0x21;		//��Ĵ���������Ϊ21����ǰ���21
	//tmp[9]=0x59;		//�ֱ����Ĵ�����ʹ�ܷ��ӱ���������һ����59���ӱ���  //0x01�ֱ���     //0x80������
	tmp[9]=0x80;
	tmp[10]=0x80;		//ʱ�����Ĵ�������ֹСʱ����
	tmp[11]=0x80;		//�챨���Ĵ�������ֹ���ڱ���
	tmp[12]=0x80;		//�ܱ����Ĵ�������ֹ�ܱ���
	tmp[13]=0x00;		//����CLKOUT����Ϊ����
	//tmp[14]=0x82;		//���ö�ʱ���������Ĵ���TE=1,TD1=1,TD0=0(1Hz��
	//tmp[15]=0x3c;		//���ö�ʱ������������ֵΪ0x3c����60s
	tmp[14]=0x03;		//���ö�ʱ���������Ĵ���TE=1,TD1=1,TD0=1(1/60Hz��
	tmp[15]=0x00;		//���ö�ʱ������������ֵΪ0xf0����14400s
	WriteNbyte(0x00, tmp, 16);			//��ʼ��pcf8563
	IP=0x01;
	IPH=0x01;				//RTC���ȼ�Ϊ3��(��߼�)
}

