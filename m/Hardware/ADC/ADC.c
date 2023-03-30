#include <ADC.h>
#include <gpioconfig.h>
#include <intrins.h>
//ADC配置
void ADC_init()
{
	ADCCFG=0X2F;//右对齐,ADC转换时间为512个时钟数
	ADC_CONTR=0X82;//打开ADC电源，选择P1.2做为ADC
}

u16 ADC_Get()
{
	u16 ADC;
	ADC_CONTR |= 0x40; // 启动ADC
	_nop_();  
  _nop_();  
	while (!(ADC_CONTR & 0x20)); //等待ADC完成 
	ADC_CONTR &= ~0x20; //	清除完成标志位 
	ADC = ((u16)ADC_RES)<<8|ADC_RESL; //读取ADC
	return ADC;
}

u16 ADC_Average()
{
	u16 sum=0;
	u16 ad,i;
	u16 average;
	for(i=0;i<9;i++)
	{
		ad=ADC_Get();
		sum=sum+ad;
	}
	average=sum/10;
	return average;
}