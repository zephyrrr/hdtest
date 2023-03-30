#include <delay.h>
#include <intrins.h>


void Delay500ms()		//@12.000MHz
{
	unsigned char i, j, k;

	_nop_();
	i = 31;
	j = 113;
	k = 29;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}

void Delay1000ms()		//@12.000MHz
{
	unsigned char i, j, k;

	_nop_();
	_nop_();
	i = 61;
	j = 225;
	k = 62;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}


void Delay1ms(u16 ms)		//@12.000MHz
{
	unsigned char i, j;
	u16 k;
	i = 16;
	j = 147;
	for(k=0;k<ms;k++)
	{
		do
		{
			while (--j);
		} while (--i);
	}
}

void Delay100ms()		//@12.000MHz
{
	unsigned char i, j, k;

	_nop_();
	i = 7;
	j = 23;
	k = 105;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}


void Delay30us()		//@12.000MHz
{
	unsigned char i;

	_nop_();
	_nop_();
	i = 87;
	while (--i);
}


