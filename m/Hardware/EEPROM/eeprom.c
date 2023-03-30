#include <eeprom.h>
#include <intrins.h>

void IapIdle()
{
		IAP_CONTR = 0; 						//禁止EEPROM操作 
		IAP_CMD = 0; 							// 执行空操作
		IAP_TRIG = 0; 						// 
		IAP_ADDRH = 0x80; 				//将地址设置到非IAP区
		IAP_ADDRL = 0; 
}
//读操作
u8 IapRead(u16 addr) 
{  
		u8 dat;   
		IAP_CONTR = WT_12M;  		//使能IAP，12M
		IAP_CMD = 1;   					//读EEPROM命令
		IAP_ADDRL = addr;   
		IAP_ADDRH = addr >> 8;	//写入目标地址 
		IAP_TRIG = 0x5a;  			//触发寄存器
		IAP_TRIG = 0xa5;  			//写入5A,A5触发相应的读写操作
		_nop_();  							//延时1us
		dat = IAP_DATA;   
		IapIdle();  						//关闭IAP
		return dat; 
} 
//写操作	
void IapWrite(u16 addr,u8 dat) 
{     
		IAP_CONTR = WT_12M; 		//使能IAP，12M 
		IAP_CMD = 2;   					//写EEPROM命令
		IAP_ADDRL = addr;   
		IAP_ADDRH = addr >> 8;	//写入目标地址 
		IAP_DATA=dat; 					//写入IAP数据
		IAP_TRIG = 0x5a;  			//触发寄存器
		IAP_TRIG = 0xa5;  			//写入5A,A5触发相应的读写操作
		_nop_();  							//延时1us	  
		IapIdle();  						//关闭IAP
} 
//擦除操作
void IapErase(u16 addr) 
{     
		IAP_CONTR = WT_12M;  		//使能IAP，12M
		IAP_CMD = 3;   					//擦除EEPROM命令
		IAP_ADDRL = addr;   
		IAP_ADDRH = addr >> 8;	//写入目标地址 
		IAP_TRIG = 0x5a;  			//触发寄存器
		IAP_TRIG = 0xa5;  			//写入5A,A5触发相应的读写操作
		_nop_();  							//延时1us	  
		IapIdle();  						//关闭IAP
}