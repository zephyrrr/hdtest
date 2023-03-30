#include <pwm.h>

//全低100%亮度，全高0%亮度
void PWM_init()
{
	P_SW2 = 0x80;  	//将EAXFR置1， 允许访问特殊功能寄存器
	PWMCKS = 0x00; 	//PWM时钟为系统时钟
	PWMC = 0x12C0; 	//设置PWM周期为12C0H个PWM时钟0X64*0X30  
	PWM0T1= 0x0000; //在计数值为000H地方输出低电平  
	PWM0T2= 0x12C0; //在计数值为12C0H地方输出高电平，100%亮度
	PWM0CR= 0xC0; 	//使能PWM0 ,初始电平为高电平  
	PWMCR = 0x80; 	//使能PWM
}

void PWM_set(u16 pwm)
{
	P_SW2 = 0x80;  	//将EAXFR置1， 允许访问特殊功能寄存器
	PWMCKS = 0x00; 	// PWM时钟为系统时钟
	PWMC = 0x12C0; 	//设置PWM周期为12C0H个PWM时钟0X64*0X30  
	PWM0T1= 0x0000; //在计数值为000H地方输出低电平  
	PWM0T2= pwm; //在计数值为12C0H地方输出高电平，100%亮度
	PWM0CR= 0xC0; 	//使能PWM0 ,初始电平为高电平  
	PWMCR = 0x80; 	//使能PWM
}