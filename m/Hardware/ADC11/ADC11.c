#include <ADC11.h>
#include <gpioconfig.h>
#include <intrins.h>
//ADC����
void ADC11_init()
{
	ADCCFG=0X2F;//�Ҷ���,ADCת��ʱ��Ϊ512��ʱ����
	ADC_CONTR=0X8B;//��ADC��Դ��ѡ��P1.2��ΪADC
}

u8 ADC11_Get()
{
	u8 ADC;
	ADC_CONTR |= 0x40; // ����ADC
	_nop_();  
  _nop_();  
	while (!(ADC_CONTR & 0x20)); //�ȴ�ADC��� 
	ADC_CONTR &= ~0x20; //	�����ɱ�־λ 
	ADC = ADC_RESL; //��ȡADC��8λ
	return ADC;
}
