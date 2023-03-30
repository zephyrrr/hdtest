#include <ADC11.h>
#include <gpioconfig.h>
#include <intrins.h>
//ADC配置
void ADC11_init()
{
	ADCCFG=0X2F;//右对齐,ADC转换时间为512个时钟数
	ADC_CONTR=0X8B;//打开ADC电源，选择P1.2做为ADC
}

u8 ADC11_Get()
{
	u8 ADC;
	ADC_CONTR |= 0x40; // 启动ADC
	_nop_();  
  _nop_();  
	while (!(ADC_CONTR & 0x20)); //等待ADC完成 
	ADC_CONTR &= ~0x20; //	清除完成标志位 
	ADC = ADC_RESL; //读取ADC低8位
	return ADC;
}
