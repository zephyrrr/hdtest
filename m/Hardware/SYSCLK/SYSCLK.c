#include <sysclk.h>
#include <stc8.h>

void sysclk_init()
{
    P_SW2 = 0x80;
    XOSCCR = 0xc0; 						//�����ⲿ����
    while (!(XOSCCR & 1)); 		//�ȴ�ʱ���ȶ�
    CLKDIV = 0x00; 						//ʱ�Ӳ���Ƶ
    CKSEL = 0x01; 						//ѡ���ⲿ����
    P_SW2 = 0x00;
    /*
    P_SW2 = 0x80;
    IRC32KCR = 0x80; 					//�����ڲ� 32K IRC
    while (!(IRC32KCR & 1)); 	//�ȴ�ʱ���ȶ�
    CLKDIV = 0x00; 						//ʱ�Ӳ���Ƶ
    CKSEL = 0x03; 						//ѡ���ڲ� 32K
    P_SW2 = 0x00;
    */
}
