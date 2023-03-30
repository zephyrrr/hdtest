#include <key.h>
#include <GPIOConfig.h>
#include <usart2.h>
#include <delay.h>

u8 test[5]={0x42,0x26,0x23};
u8 test1[5]={0x24,0x52,0x69};
void KEY_init()
{
	INTCLKO = 0x30; //使能INT2和INT3中断 
}

void INT2Isr()   interrupt 10 using 1
{
	if(KEY0==0)
	{
		delay_ms(10);			//消抖
		if(KEY0==0)
		{
			LED2=0;	
			Uart2SendStr(test,sizeof(test));
		}
	}
}


void INT3Isr()   interrupt 11 using 1
{
	if(KEY1==0)
	{
		delay_ms(10);
		if(KEY1==0)
		{
			LED1=0;
			Uart2SendStr(test1,sizeof(test1));
		}
	}
}