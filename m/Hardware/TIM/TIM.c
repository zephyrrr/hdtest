#include <TIM.h>
#include <gpioconfig.h>
#include <intrins.h>
#include <STC8.h>
#include <math.h>
#include <pwm.h>
#include "adc.h"
#include <usart2.h>
#include <stdio.h>
u16 voldata[100];
bit adc_flag=0;
//��ʱ������
void Timer0Init(void)		//200΢��@12.000MHz
{
	AUXR &= 0x7F;		//��ʱ��ʱ��12Tģʽ
	TMOD &= 0xF0;		//���ö�ʱ��ģʽ
	TL0 = 0x38;		//���ö�ʱ��ʼֵ
	TH0 = 0xFF;		//���ö�ʱ��ʼֵ
	TF0 = 0;		//���TF0��־
	TR0 = 1;		//��ʱ��0��ʼ��ʱ
	ET0=1; //ʹ�ܶ�ʱ���ж� 
	EA = 1; 
}

void TM0_Isr() interrupt 1 using 1 
{ 
	static count=0;
	voldata[count]=ADC_Get();
	count++;
	if(count==100) //һ������
	{
		count=0;
		ET0=0; //��ֹ��ʱ���ж� 
		adc_flag=1;   //һ�β�����ɱ�־
	}	 
 TF0 = 0;//���жϱ�־ 
}  


u16 RMS_count(u16 *vol,u16 sizet)
{
	u16 RMS=0;
	u32 sum=0;
	u16 i;
	//�жϵ�ѹ��С
	for(i=0;i<sizet;i++)
	{
		if(vol[i]<=0x0800)	vol[i]=0x0800-vol[i];
		else vol[i]-=0x0800;
	}
	for(i=0;i<sizet;i++)
	{
		sum+=pow(vol[i],2);
	}
	RMS=sqrt(sum/100);
	return RMS;
}

//1�ε�ѹ��Чֵ
u16 RMS_ONE(void)
{
	u16 outRMS=0;
	Timer0Init();
	while(!adc_flag);
	outRMS=RMS_count(voldata,100);
	adc_flag=0;			//���һ�β�����ɱ�־
	return outRMS;
}

//5�ε�ѹ��Чֵ��ƽ��
u16 RMS_Average(void)
{
	int i=0;
	u16 sum=0;
	for(i=0;i<5;i++)
	{
		Timer0Init();
		while(!adc_flag);
		sum+=RMS_count(voldata,100);
		adc_flag=0;			//���һ�β�����ɱ�־
	}
	return sum/5;
}

