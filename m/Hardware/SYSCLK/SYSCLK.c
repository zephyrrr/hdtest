#include <sysclk.h>
#include <stc8.h>

void sysclk_init()
{
    P_SW2 = 0x80;
    XOSCCR = 0xc0; 						//启动外部晶振
    while (!(XOSCCR & 1)); 		//等待时钟稳定
    CLKDIV = 0x00; 						//时钟不分频
    CKSEL = 0x01; 						//选择外部晶振
    P_SW2 = 0x00;
    /*
    P_SW2 = 0x80;
    IRC32KCR = 0x80; 					//启动内部 32K IRC
    while (!(IRC32KCR & 1)); 	//等待时钟稳定
    CLKDIV = 0x00; 						//时钟不分频
    CKSEL = 0x03; 						//选择内部 32K
    P_SW2 = 0x00;
    */
}
