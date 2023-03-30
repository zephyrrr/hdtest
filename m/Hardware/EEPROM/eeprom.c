#include <eeprom.h>
#include <intrins.h>

void IapIdle()
{
		IAP_CONTR = 0; 						//��ֹEEPROM���� 
		IAP_CMD = 0; 							// ִ�пղ���
		IAP_TRIG = 0; 						// 
		IAP_ADDRH = 0x80; 				//����ַ���õ���IAP��
		IAP_ADDRL = 0; 
}
//������
u8 IapRead(u16 addr) 
{  
		u8 dat;   
		IAP_CONTR = WT_12M;  		//ʹ��IAP��12M
		IAP_CMD = 1;   					//��EEPROM����
		IAP_ADDRL = addr;   
		IAP_ADDRH = addr >> 8;	//д��Ŀ���ַ 
		IAP_TRIG = 0x5a;  			//�����Ĵ���
		IAP_TRIG = 0xa5;  			//д��5A,A5������Ӧ�Ķ�д����
		_nop_();  							//��ʱ1us
		dat = IAP_DATA;   
		IapIdle();  						//�ر�IAP
		return dat; 
} 
//д����	
void IapWrite(u16 addr,u8 dat) 
{     
		IAP_CONTR = WT_12M; 		//ʹ��IAP��12M 
		IAP_CMD = 2;   					//дEEPROM����
		IAP_ADDRL = addr;   
		IAP_ADDRH = addr >> 8;	//д��Ŀ���ַ 
		IAP_DATA=dat; 					//д��IAP����
		IAP_TRIG = 0x5a;  			//�����Ĵ���
		IAP_TRIG = 0xa5;  			//д��5A,A5������Ӧ�Ķ�д����
		_nop_();  							//��ʱ1us	  
		IapIdle();  						//�ر�IAP
} 
//��������
void IapErase(u16 addr) 
{     
		IAP_CONTR = WT_12M;  		//ʹ��IAP��12M
		IAP_CMD = 3;   					//����EEPROM����
		IAP_ADDRL = addr;   
		IAP_ADDRH = addr >> 8;	//д��Ŀ���ַ 
		IAP_TRIG = 0x5a;  			//�����Ĵ���
		IAP_TRIG = 0xa5;  			//д��5A,A5������Ӧ�Ķ�д����
		_nop_();  							//��ʱ1us	  
		IapIdle();  						//�ر�IAP
}