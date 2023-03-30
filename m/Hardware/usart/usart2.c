#include <usart2.h>
#include <GPIOConfig.h>
#include <eeprom.h>
#include <I2C.h>
#include <stdio.h>
#include <Delay.h>
#include <eeprom.h>
#define FOSC 12000000UL 
#define BRT1 (65536 - FOSC/9600/4)
#define BRT2 (65536 - FOSC/115200/4)
bit busy;
u8 Usart2_Rx_Buf[20]; 
u8 ACK[4];
u8 NOACK[4];
bit CRC_OK;
u8 Usart2_Rx_Sum;
//����2��ʼ��
void Uart2_Init_9600()
{
	S2CON = 0x50; 
	T2L = BRT1;  //�����Ĵ�����8λ
	T2H = BRT1 >> 8; //�����Ĵ�����8λ
	IP2=0X00;
	IP2H=0X01;		//���ȼ�Ϊ2�����ϸ߼���
	AUXR = 0x14; //��ʱ��2�����Ĵ���������Ƶ����ʼ����
} 
void Uart2_Init_115200()
{
	S2CON = 0x50; //ģʽ0�������ڽ���
	T2L = BRT2;  //�����Ĵ�����8λ
	T2H = BRT2 >> 8; //�����Ĵ�����8λ
	IP2=0X00;
	IP2H=0X01;		//���ȼ�Ϊ2�����ϸ߼���
	AUXR = 0x14; //��ʱ��2�����Ĵ���������Ƶ����ʼ����
	IE2 = 0x01;//ʹ�ܴ����ж�
}

//�����жϷ�����
void Uart2Isr()   interrupt 8 using 2
{
	static u8 first=0;
	static u8 second=0;
	static u8 third=0;
	static u8 Usart2_Rx_Num=0;
	static u8 j=0;
	if(S2CON&0x01)//�����ж�
	{		
		first=second;
		second=third;
		third=S2BUF;
		if(first==0xA5&&(second==0xB2||second==0xB3||second==0xB5||second==0xB6||second==0xB7||second==0xB8||second==0xBB||second==0xBD||second==0xBE))//�ж�֡ͷ��������
		{	
			Usart2_Rx_Buf[0]=second;//����������
			Usart2_Rx_Buf[1]=third;	//�����ֽ���
			Usart2_Rx_Num=2;
			
		}
		else if(Usart2_Rx_Buf[0]==0xB2||Usart2_Rx_Buf[0]==0xB3||Usart2_Rx_Buf[0]==0xB5||Usart2_Rx_Buf[0]==0xB6||Usart2_Rx_Buf[0]==0xB7||Usart2_Rx_Buf[0]==0xB8||Usart2_Rx_Buf[0]==0xBB||Usart2_Rx_Buf[0]==0xBD||Usart2_Rx_Buf[0]==0xBE)
		{			
			Usart2_Rx_Buf[Usart2_Rx_Num]=third;
			Usart2_Rx_Num++;
			if(Usart2_Rx_Num==Usart2_Rx_Buf[1]+3)													//���ݽ������
			{			
				Usart2_Rx_Num=0;	//���ݳ�������
				Usart2_Rx_Sum=0;//���У��				
				for(j=0;j<Usart2_Rx_Buf[1]+2;j++)
				{
					Usart2_Rx_Sum+=Usart2_Rx_Buf[j];
				}	
				Usart2_Rx_Sum+=0xA5;
				//��ȷӦ��
				ACK[0]=Usart2_Rx_Buf[0];
				ACK[1]=0x01;
				ACK[2]=0x10;
				ACK[3]=ACK[0]+ACK[1]+ACK[2];
				//����Ӧ��
				NOACK[0]=Usart2_Rx_Buf[0];
				NOACK[1]=0x01;
				NOACK[2]=0x11;
				NOACK[3]=NOACK[0]+NOACK[1]+NOACK[2];
				if(Usart2_Rx_Sum==Usart2_Rx_Buf[Usart2_Rx_Buf[1]+2])//���У����ȷ
				{
					CRC_OK=1;//У����ȷ��־
					if(Usart2_Rx_Buf[0]==0xB6||Usart2_Rx_Buf[0]==0xB3||Usart2_Rx_Buf[0]==0xB5||Usart2_Rx_Buf[0]==0xB7||Usart2_Rx_Buf[0]==0xB8||Usart2_Rx_Buf[0]==0xBB||Usart2_Rx_Buf[0]==0xBD||Usart2_Rx_Buf[0]==0xBE)
					{	
						LED1=0;
						IE2 = 0x00;			//��ֹ�����ж�
					}
				}	
				else
				{
					CRC_OK=0;
					//Usart2_Rx_Buf����				
					for(j=0;j<Usart2_Rx_Buf[1]+3;j++)		
					{
						Usart2_Rx_Buf[j]=0;
					}
					//Uart2SendStr(NOACK,sizeof(NOACK));
				}
			}					
		}				
		S2CON&=~0x01;//��������жϱ�־λ
	}		
}							
			

//���ڷ��͵����ֽ�
void Uart2Send(u8 dat)
 {
	 IE2 = 0x00;//�ر�ʹ���ж�
	 S2BUF =dat;
	 while(!(S2CON&0x02));
	 S2CON&=~0x02;
	 IE2 = 0x01;//ʹ�ܽ����ж�	 
 } 

//���ڷ����ַ��� 
void Uart2SendStr(u8 *p,u8 i)
{ 
	u8 j;
	for(j=0;j<i;j++) 
	{ 
		Uart2Send(*p++); 
	}	
} 

void putchar(char c)
{
	IE2 = 0x00;
	S2BUF =c;
	while(!(S2CON&0x02));
	S2CON&=~0x02; 
	IE2 = 0x01;
}

//LoRaģ������(�����޸Ĳ�����)
void LoRa_Config()
{
	LED3=0;
	Uart2_Init_115200();
	Delay100ms();
	printf("+++");
	Delay100ms();
	printf("AT+TSF=07\r\n");
	Delay100ms();
	printf("AT+RSF=07\r\n");
	Delay100ms();
	printf("AT+TPREM=0028\r\n");
	Delay100ms();
	printf("AT+RPREM=0028\r\n");
	Delay100ms();
	printf("AT+SIP=01\r\n");
	Delay100ms();
	//�����µ�Ƶ�Σ�����Ƶ��505.5MHz�������Ƶ473.5MHz
	printf("AT+TFREQ=1C390960\r\n");
	Delay100ms();
	printf("AT+RFREQ=1E215160\r\n");
	Delay100ms();
//	printf("AT+BRATE=07\r\n");
//	Delay100ms();
//	Uart2_Init_115200();
//	Delay100ms();
//	printf("+++");
//	Delay100ms();
	printf("ATT\r\n");
	Delay100ms();
}

