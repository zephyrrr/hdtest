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
//定时器配置
void Timer0Init(void)		//200微秒@12.000MHz
{
	AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xF0;		//设置定时器模式
	TL0 = 0x38;		//设置定时初始值
	TH0 = 0xFF;		//设置定时初始值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
	ET0=1; //使能定时器中断 
	EA = 1; 
}

void TM0_Isr() interrupt 1 using 1 
{ 
	static count=0;
	voldata[count]=ADC_Get();
	count++;
	if(count==100) //一个周期
	{
		count=0;
		ET0=0; //禁止定时器中断 
		adc_flag=1;   //一次采样完成标志
	}	 
 TF0 = 0;//清中断标志 
}  


u16 RMS_count(u16 *vol,u16 sizet)
{
	u16 RMS=0;
	u32 sum=0;
	u16 i;
	//判断电压大小
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

//1次电压有效值
u16 RMS_ONE(void)
{
	u16 outRMS=0;
	Timer0Init();
	while(!adc_flag);
	outRMS=RMS_count(voldata,100);
	adc_flag=0;			//清除一次采样完成标志
	return outRMS;
}

//5次电压有效值求平均
u16 RMS_Average(void)
{
	int i=0;
	u16 sum=0;
	for(i=0;i<5;i++)
	{
		Timer0Init();
		while(!adc_flag);
		sum+=RMS_count(voldata,100);
		adc_flag=0;			//清除一次采样完成标志
	}
	return sum/5;
}

