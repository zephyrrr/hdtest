#include <ADC.h>
#include <gpioconfig.h>
#include <intrins.h>
//ADC����
void ADC_init()
{
	ADCCFG=0X2F;//�Ҷ���,ADCת��ʱ��Ϊ512��ʱ����
	ADC_CONTR=0X82;//��ADC��Դ��ѡ��P1.2��ΪADC
}

u16 ADC_Get()
{
	u16 ADC;
	ADC_CONTR |= 0x40; // ����ADC
	_nop_();  
  _nop_();  
	while (!(ADC_CONTR & 0x20)); //�ȴ�ADC��� 
	ADC_CONTR &= ~0x20; //	�����ɱ�־λ 
	ADC = ((u16)ADC_RES)<<8|ADC_RESL; //��ȡADC
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