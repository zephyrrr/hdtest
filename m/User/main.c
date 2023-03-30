/*****************************************************************
项目名称：智慧照明Lora2.0，污水处理厂区
补充：0x0000 当天规则
			0x0200 下日规则
			0x0400 实时控制亮度
			0x0600 组号
			0x0800 lora配置标志位
作者：张奕雯
修改日期：2022年10月10日

*****************************************************************/
#include <STC8.H>
#include <stdio.h>
#include <intrins.h>
#include <sysclk.h>
#include <gpioconfig.h>
#include <Delay.h>
#include "usart2.h"
#include "ADC.h"
#include "I2C.h"
#include "pwm.h"
#include "eeprom.h"
#include "ADC11.h"
#include "TIM.h"

void set_gain(void);
void real_control(u8 control_light,u8 ack);
void today_rule(u8 size_t,u8 * rule,u8 ack);
void tomorrow_rule(u8 size_t,u8 * rule,u8 ack);

const u16 OPEN=0x005A;   //空载时放大21倍大致在0x000D左右，针对20w以上的灯具,但可能(暂定该值，对于不同功率的灯还需调整)
u8 request[5];													//请求入网指令
u8 no_open[4]={0xB2,0x01,0x65,0x18};		//无法开灯故障
u8 right[4];														//正常
u8 no_acheive[4]={0xB2,0x01,0x67,0x1A};	//未达到指定亮度	（未正常关灯）
u8 clk_reset[2]={0x00,0x00};						//规则更新时间
u8 cal_ok[4]={0xB2,0x01,0xFE,0xB1};			//完成时钟校准
u8 real_ok[5];    											//标志日志
u8 time_set[6];													//时间校准
u8 clk_time[2]=0;
u8 current_rule[20];				//当天时间规则（每天24点更新后记录）
u8 next_rule[20];
u8 current_number;					//当天闹钟数（每天24点更新）
u8 current_time[2];				  //当前时间	
u16 light;									//亮度切换时记录亮度
u8 next_clk[2];
bit calibration_flag;       //时钟校准标志位
bit group_flag;							//组播标志位
bit day_clk;								//规则日志标志
bit real_flag;							//实时日志标志
bit first_b7;								//入网时间校准位
bit B2_flag;								//收到B2标志位	
bit res_flag;								//组播规则标志位
bit request_flag;

int main(void)
{	
	
	u8 i;
	u8 TIME[2];
	u16 addr;
	u8 seed;		//入网时间种子
	u16 adc;
	u16 j;
	u8 power;		//拨码开关
	u16 powerdata=0;//电压有效值
	CRC_OK=0;
	sysclk_init();
	gpioconfig();
	PWM_init();	
	ADC11_init();	
	Uart2_Init_115200();
	IT0=1;			//使能INT0下降沿
	EX0=1;			//使能INT0中断
	EA = 1;	
	addr=0;	
	relay=1;										//继电器闭合
	request_flag=0;							//入网请求标志位
  calibration_flag=0;					//时钟校准标志位
	group_flag=0;
	INT_flag=0;			            //RTC 中断标志位
	first_b7=1;									//入网校准标志位
	res_flag=1;									//Lora模块复位标志
	B2_flag=0;
	crl=1;											//2倍放大（初始化电流小倍数放大）
	adc_flag=0;									//AD周期采集结束标志
	light=0x64;
	day_clk=0;
	real_flag=0;
	CRC_OK=0;
	power=((P0&0x10)>>4)*8+((P0&0x20)>>5)*4+((P0&0x40)>>6)*2+((P0&0x80)>>7);//4位拨码（功能待定）
	rest=1;
	//清空当前规则数组
	for(j=0;j<20;j++)
	{
		current_rule[j]=0xff;
	}
	seed=ADC11_Get();						//获取随机种子
	if(seed<0x64)
		seed+=0x64;
	ADC_init();	
	set_gain();	//选择放大倍数（2倍或21倍）
	Delay1ms(seed*2);	//随机延时
	RTC_init();
	//IIC通信测试
//	ReadNbyte(0x03,current_time,2);	
//	current_time[0]&=0x7f;//分
//	current_time[1]&=0x3f;//时	
//	Uart2SendStr(current_time,sizeof(current_time));
	powerdata=RMS_Average();
	//printf("%4x",powerdata);
	//判断放大倍数
	if(crl)//放大2倍
	{
		LED2=0;
		powerdata/=2;
	}
	else if(!crl)
		powerdata/=21;
	//请求入网指令
	request[0]=0xBA;
	request[1]=0x02;
	request[2]=powerdata>>8;
	request[3]=(u8)powerdata;
	request[4]=request[0]+request[1]+request[2]+request[3];
	if(IapRead(0x0800)==0xff)
	{
		LoRa_Config();			//Lora模块配置	
		LED3=1;
		IapWrite(0x0800,0x01);
	}
	WDT_CONTR=0x27;                  //启动看门狗，8s
	while(1) 
		{		
			WDT_CONTR|=0x10;					//清看门狗，否则系统复位
			//成功入网
			if(request_flag)
			{ 		
				LED2=1;					//LED2--规则更新指示灯
				LED1=1;					//LED1--规则时间点切换指示灯
				LED0=~LED0;			//入网运行指示灯
				Delay500ms();
				ReadNbyte(0x03,current_time,2);	//查看当前时间
				current_time[0]&=0x7f;//分
				current_time[1]&=0x3f;//时
				//隔2min复位Lora模块
				if(current_time[0]%2==0&&res_flag)
				{				
						//复位模块
						rest=0;
						Delay30us();
						rest=1;
						res_flag=0;					
				}	
				if(current_time[0]%2!=0&&!res_flag)
				{
					res_flag=1;
				}															
				//上发命令
				if(CRC_OK)//求和校验正确
				{		
						CRC_OK=0;					//清校验正确标志位					
						if(Usart2_Rx_Buf[0]==0xB2)						//请求设备状态上传
						{											
							u8 c=0;	
							B2_flag=1;			//收到B2标志位				
							ReadNbyte(0x03,current_time,2);	
							current_time[0]&=0x7f;//分
							current_time[1]&=0x3f;//时
							WDT_CONTR|=0x10;					//清看门狗，否则系统复位
							//组播规则标志
							if(group_flag)
							{
								real_ok[0]=0xB2;
								real_ok[1]=0x02;
								real_ok[2]=0xFB;
								real_ok[3]=light;
								real_ok[4]=real_ok[0]+real_ok[1]+real_ok[2]+real_ok[3];
								Uart2SendStr(real_ok,sizeof(real_ok));
								group_flag=0;
							}
							//时钟校准标志
							else if(calibration_flag)
							{							
								Uart2SendStr(cal_ok,sizeof(cal_ok));
								calibration_flag=0;
							}
							//规则日志标志
							else if(day_clk)				
							{
								real_ok[0]=0xB2;
								real_ok[1]=0x02;
								real_ok[2]=0xFD;
								real_ok[3]=light;
								real_ok[4]=real_ok[0]+real_ok[1]+real_ok[2]+real_ok[3];
								Uart2SendStr(real_ok,sizeof(real_ok));
								day_clk=0;
							}
							//实时日志标志
							else if(real_flag)				
							{
								real_ok[0]=0xB2;
								real_ok[1]=0x02;
								real_ok[2]=0xFC;
								real_ok[3]=light;
								real_ok[4]=real_ok[0]+real_ok[1]+real_ok[2]+real_ok[3];
								Uart2SendStr(real_ok,sizeof(real_ok));
								real_flag=0;
							}
							//当前正在执行规则模式
							else if(IapRead(0x0400)==0xff)							
							{
								//有规则
								if(current_rule[0]!=0xff)
								{
									right[0]=0xB2;
									right[1]=0x01;
									right[2]=light;
									right[3]=right[0]+right[1]+right[2];
									adc=RMS_Average();	//采集电压有效值
									//大于最后一个闹钟时间时
									if(current_time[1]>current_rule[1+3*(current_number-1)]||(current_time[1]==current_rule[1+3*(current_number-1)]&&current_time[0]>current_rule[3*(current_number-1)]))
									{														
										if(current_rule[2+3*(current_number-1)]>0)									//如果规则亮度不为0
										{
											if(adc>OPEN)
												Uart2SendStr(right,sizeof(right));
											else
												Uart2SendStr(no_open,sizeof(no_open));
										}
										else
										{
											if(adc<=OPEN)
												Uart2SendStr(right,sizeof(right));
											else
												Uart2SendStr(no_acheive,sizeof(no_acheive));
										}
									}																																											
									//大于第一个闹钟小于最后一个闹钟时间时
									else if((current_time[1]>current_rule[1]||(current_time[1]==current_rule[1]&&current_time[0]>current_rule[0]))&&(current_time[1]<current_rule[1+3*(current_number-1)]||(current_time[1]==current_rule[1+3*(current_number-1)]&&current_time[0]<current_rule[3*(current_number-1)])))
									{						
										for(c=0;c<current_number-1;c++)
										{
											if((current_time[1]>current_rule[1+3*c]||(current_time[1]==current_rule[1+3*c]&&current_time[0]>current_rule[3*c]))&&(current_time[1]<current_rule[4+3*c]||(current_time[1]==current_rule[4+3*c]&&current_time[0]<current_rule[3+3*c])))					//当前时间大于前一个闹钟时小于后一个闹钟时
											{								
												if(current_rule[2+3*c]>0x00&&current_rule[2+3*c]<=0x64)			//规则亮度不为0
												{
													if(adc>OPEN)
														Uart2SendStr(right,sizeof(right));
													else
														Uart2SendStr(no_open,sizeof(no_open));
												}
												else if(current_rule[2+3*c]==0x00)													//规则亮度为0
												{
													if(adc>OPEN)
														Uart2SendStr(no_acheive,sizeof(no_acheive));
													else
														Uart2SendStr(right,sizeof(right));
												}
											}																
										}																		
									}
								}
								//没有规则
								else 
								{
									right[0]=0xB2;
									right[1]=0x01;
									right[2]=0x64;
									right[3]=right[0]+right[1]+right[2];
									if(adc>OPEN)
									{
										Uart2SendStr(right,sizeof(right));
									}
									else
										Uart2SendStr(no_open,sizeof(no_open));
								}								
							}
							//当前正在执行实时模式
							else if(IapRead(0x0400)!=0xff)
							{
								light=IapRead(0x0400);
								right[0]=0xB2;
								right[1]=0x01;
								right[2]=light;
								right[3]=right[0]+right[1]+right[2];
								adc=RMS_Average();	//采集电压有效值
								//printf("adc:%4x\r\n",adc);
								if(light==0)
								{
									if(adc<=OPEN)
										Uart2SendStr(right,sizeof(right));
									else
										Uart2SendStr(no_acheive,sizeof(no_acheive));
								}
								else if(light>0x00&&light<=0x64)
								{
									if(adc<=OPEN)
										Uart2SendStr(no_open,sizeof(no_open));
									else
										Uart2SendStr(right,sizeof(right));
								}
							}
							//Usart2_Rx_Buf清零
							for(j=0;j<Usart2_Rx_Buf[1]+3;j++)		
							{
								Usart2_Rx_Buf[j]=0;
							}
						}									
						else if(Usart2_Rx_Buf[0]==0xB3)						//开关灯
						{
							//实时开关灯命令，进入实时模式						
							if((Usart2_Rx_Buf[2]>=0x00&&Usart2_Rx_Buf[2]<=0x64)||Usart2_Rx_Buf[2]==0xff)
							{
								real_control(Usart2_Rx_Buf[2],1);
							}
							else 
								Uart2SendStr(NOACK,sizeof(NOACK));
							//Usart2_Rx_Buf清零				
							for(j=0;j<Usart2_Rx_Buf[1]+3;j++)		
							{
								Usart2_Rx_Buf[j]=0;
							}
							IE2 = 0x01;			//使能串口中断						
						}
						else if(Usart2_Rx_Buf[0]==0xB5)						//规则接收（分/时/亮度）下一天的规则
						{
							u8 rule[20];
							for(j=0;i<Usart2_Rx_Buf[1];j++)
							{
								rule[j]=Usart2_Rx_Buf[2+j];
							}
							tomorrow_rule(Usart2_Rx_Buf[1],rule,1);
							//Usart2_Rx_Buf清零				
							for(j=0;j<Usart2_Rx_Buf[1]+3;j++)		
							{
								Usart2_Rx_Buf[j]=0;
							}
							IE2 = 0x01;			//使能串口中断
						}
						else if(Usart2_Rx_Buf[0]==0xB6)						//规则修改（修改当天规则）
						{
							u8 rule[20];
							for(j=0;j<Usart2_Rx_Buf[1];j++)
							{
								rule[j]=Usart2_Rx_Buf[2+j];
							}
							today_rule(Usart2_Rx_Buf[1],rule,1);
							//Usart2_Rx_Buf清零				
							for(j=0;j<Usart2_Rx_Buf[1]+3;j++)		
							{
								Usart2_Rx_Buf[j]=0;
							}
							IE2 = 0x01;			//使能串口中断
						}
						else if(Usart2_Rx_Buf[0]==0xB7)						//时钟校准
						{
							if(Usart2_Rx_Buf[1]==0x08)
							{
								//写入校准时间
								for(i=0;i<6;i++)
								{						
									time_set[i]=Usart2_Rx_Buf[2+i];								
								}
								WriteNbyte(0x02, time_set, 7);
								calibration_flag=1;						//校准标志位置1
								LED3=0;
								Delay500ms();	
								LED3=1;									
							}
							else
								Uart2SendStr(NOACK,sizeof(NOACK));							
							//Usart2_Rx_Buf清零				
							for(j=0;j<Usart2_Rx_Buf[1]+3;j++)		
							{
								Usart2_Rx_Buf[j]=0;
							}
							IE2 = 0x01;			//使能串口中断						
						}																					
						else if(Usart2_Rx_Buf[0]==0xB8)						//分组命令
						{
							//记录组号							
							IapErase(0x0600);//擦除组号
							if(Usart2_Rx_Buf[2]!=0xff)
								IapWrite(0x0600,Usart2_Rx_Buf[2]);//写入组号
							Uart2SendStr(ACK,sizeof(ACK));
							//Usart2_Rx_Buf清零				
							for(j=0;j<Usart2_Rx_Buf[1]+3;j++)		
							{
								Usart2_Rx_Buf[j]=0;
							}
							IE2 = 0x01;			//使能串口中断	
						}
						else if(Usart2_Rx_Buf[0]==0xBB)						//组播实时控制命令
						{
							if(Usart2_Rx_Buf[2]==IapRead(0x0600))		//查看组号是否匹配		
							{
								if((Usart2_Rx_Buf[3]>=0x00&&Usart2_Rx_Buf[3]<=0x64)||Usart2_Rx_Buf[3]==0xff)
								{
									real_control(Usart2_Rx_Buf[3],0);
								}
//								else 
//									Uart2SendStr(NOACK,sizeof(NOACK));
							}
							//Usart2_Rx_Buf清零				
							for(j=0;j<Usart2_Rx_Buf[1]+3;j++)		
							{
								Usart2_Rx_Buf[j]=0;
							}
							IE2 = 0x01;			//使能串口中断	
						}							
						else if(Usart2_Rx_Buf[0]==0xBD)						//组播次日规则命令
						{
							if(Usart2_Rx_Buf[2]==IapRead(0x0600))		//查看组号是否匹配		
							{
								u8 rule[20];
								for(j=0;j<Usart2_Rx_Buf[1]-1;j++)//获取规则
								{
									rule[j]=Usart2_Rx_Buf[3+j];
								}
								tomorrow_rule(Usart2_Rx_Buf[1]-1,rule,0);
								group_flag=1;		//组播标志位
								day_clk=0;
							}
							//Usart2_Rx_Buf清零				
							for(j=0;j<Usart2_Rx_Buf[1]+3;j++)		
							{
								Usart2_Rx_Buf[j]=0;
							}
							IE2 = 0x01;			//使能串口中断
						}
						else if(Usart2_Rx_Buf[0]==0xBE)						//组播当日规则命令
						{
							if(Usart2_Rx_Buf[2]==IapRead(0x0600))		//查看组号是否匹配		
							{
								u8 rule[20];
								for(j=0;j<Usart2_Rx_Buf[1]-1;j++)//获取规则
								{
									rule[j]=Usart2_Rx_Buf[3+j];
								}
								today_rule(Usart2_Rx_Buf[1]-1,rule,0);//不作应答，等下次轮询上报
								group_flag=1;		//组播标志位
								day_clk=0;
							}
							//Usart2_Rx_Buf清零				
							for(j=0;j<Usart2_Rx_Buf[1]+3;j++)		
							{
								Usart2_Rx_Buf[j]=0;
							}
							IE2 = 0x01;			//使能串口中断
						}
						else
						{
							//Usart2_Rx_Buf清零				
							for(j=0;j<Usart2_Rx_Buf[1]+3;j++)		
							{
								Usart2_Rx_Buf[j]=0;
							}
						}
				}
				//判断RTC定时中断
				if(INT_flag)
				{
					u8 clear_1=0x02;
					//判断是否是设定的闹钟 （查看当前时间）
					ReadNbyte(0x03,TIME,2);
					Delay100ms();
					TIME[0]&=0x7f;//分
					TIME[1]&=0x3f;//时
					INT_flag=0;
					Delay500ms();
					WriteNbyte(0x01,&clear_1,1); 								//清除RTC定时中断标志													
					//如果是24点的闹钟，则更新新一天的闹钟
					if(TIME[0]==0x00&&TIME[1]==0x00)
					{		
						LED2=0;	
						Delay500ms();						
						//替换规则
						IapErase(0x0000);													//擦除上一天的规则	
						for(addr=0;addr<20;addr++)					//写入当天规则分/时/亮度(当天的规则写在第一页）
						{							
							next_rule[addr]=IapRead(0x0200+addr);
						}						
						for(addr=0;addr<20;addr++)					//写入当天规则分/时/亮度(当天的规则写在第一页）
						{							
							IapWrite(addr,next_rule[addr]);
						}
						for(addr=0;addr<20;addr++)		
						{
							current_rule[addr]=IapRead(addr);					//更新当前规则，分/时/亮度
						}
						addr=0;
						while(current_rule[addr]!=0xff)
						{
							addr++;
						}	
						current_number=addr/3;													//记录当前闹钟个数	
						//Uart2Send(current_number);						
						//当前为规则模式
						if(IapRead(0x0400)==0xff)
						{
							day_clk=1;		//日志标志位
							//有规则
							if(current_rule[0]!=0xff)
							{
								//执行当天第一个闹钟（0点）
								light=current_rule[2];
								if(light==0) relay=0;
								else
								{				
									PWM_set(light*0x0030);							
									relay=1;
								}
								if(current_number>=2)
								{
									//设置当天第二个闹钟					
								clk_time[0]=current_rule[3];
								clk_time[1]=current_rule[4];					
								WriteNbyte(0x09,clk_time,2);
								//Uart2SendStr(clk_time,sizeof(clk_time));
								}
								else
									WriteNbyte(0x09,clk_reset,2);
							}
							//没有规则
							else if(current_rule[0]==0xff)
							{
								light=0x64;
								PWM_set(0x12C0);							
								relay=1;							
							}							
						}	
						//当前为实时模式
						else if(IapRead(0x0400)!=0xff)
						{
							light=IapRead(0x0400);
							if(light==0x00) relay=0;
							else
							{
								PWM_set(light*0x0030);
								relay=1;
							}		
						}					
					}
					//如果不是24点的闹钟
					else
					{	
						day_clk=1;		//日志标志位
						LED1=0;
						Delay500ms();	
						for(j=1;j<current_number;j++)
						{
							if(TIME[0]==current_rule[3*j]&&TIME[1]==current_rule[1+3*j])
							{
								light=current_rule[2+3*j];
								if(j==current_number-1)//最后一个时间段
								{
									if(light==0)		relay=0;
									else
									{
										PWM_set(light*0x0030);									
										relay=1;
									}
									WriteNbyte(0x09,clk_reset,2);
									Delay100ms();
									//ReadNbyte(0x09,next_clk, 2);
									//Uart2SendStr(next_clk,2);
									WriteNbyte(0x01,&clear_1,1);
								}
								else
								{	
									if(light==0) 
									{
										relay=0;
									}
									else
									{
										PWM_set(light*0x0030);									
										relay=1;
									}
									//设置下一个闹钟	
									clk_time[0]=current_rule[3+3*j];
									clk_time[1]=current_rule[4+3*j];														
									WriteNbyte(0x09,clk_time,2);
									//Uart2SendStr(clk_time,sizeof(clk_time));
								}
							}
						}	
						//WriteNbyte(0x01,&tmp[1],1);
					}						
					WriteNbyte(0x01,&clear_1,1); 								//清除RTC定时中断标志	
					IE2=0x01;
				}				
			}
			//未入网成功，LED0常亮，继续请求入网，照明灯常亮
			else
			{								
				//照明灯亮（没有规则且实时指令）
				LED0=0;				
				Uart2SendStr(request,sizeof(request));				//等待入网
				Delay1ms(seed);
				WDT_CONTR|=0x10;					//清看门狗，否则系统复位
				Delay1ms(seed);
				WDT_CONTR|=0x10;					//清看门狗，否则系统复位
				//判断是否是上电后第一次时间校准（作为入网）
				if(CRC_OK)
				{
					CRC_OK=0;
					if(first_b7&&Usart2_Rx_Buf[0]==0xB7&&Usart2_Rx_Buf[1]==0x08)
					{
					u8 h=0;
					u8 flag_mod;//查看是否是实时模式
					//写入校准时间
					for(i=0;i<6;i++)
					{						
						time_set[i]=Usart2_Rx_Buf[2+i];								
					}
					WriteNbyte(0x02, time_set, 7);
					//从EEPROM中读取当前规则和数量	
					for(j=0;j<20;j++)
					{
						current_rule[j]=IapRead(j);
					}
					while(current_rule[h]!=0xff)		 
					{										
						h++;									
					}
					//测试IIC通信是否正常
//					ReadNbyte(0x03,current_time,2);	
//					current_time[0]&=0x7f;//分
//					current_time[1]&=0x3f;//时	
//					Uart2SendStr(current_time,sizeof(current_time));
					current_number=h/3;				//记录规则数量
					flag_mod=IapRead(0x0400);	//查看是否是实时模式
					//执行规则模式
					if(IapRead(0x0400)==0xff)
					{
						day_clk=1;		//规则日志标志位																				
						//入网后没有规则灯常亮(%100)
						if(current_rule[0]==0xff)
						{
							relay=1;
							light=0x64;
							WriteNbyte(0x09,clk_reset,2);
						}	
						//入网后有规则按规则走
						else if(current_rule[0]!=0xff)
						{
							//查看当前时间
							ReadNbyte(0x03,current_time,2);	
							current_time[0]&=0x7f;//分
							current_time[1]&=0x3f;//时	
							//当前时间大于等于最后一个闹钟时
							if(current_time[1]>current_rule[1+3*(current_number-1)]||(current_time[1]==current_rule[1+3*(current_number-1)]&&current_time[0]>=current_rule[3*(current_number-1)]))
							{		
								light=current_rule[2+3*(current_number-1)];
								if(light==0x00)			relay=0;													
								else
								{ 
									PWM_set(light*0x0030);									
									relay=1;																						
								}
								WriteNbyte(0x09, clk_reset,2);											
							}	
							//当前时间大等于于第一个闹钟小于最后一个闹钟时
							else if((current_time[1]>current_rule[1]||(current_time[1]==current_rule[1]&&current_time[0]>=current_rule[0]))&&(current_time[1]<current_rule[1+3*(current_number-1)]||(current_time[1]==current_rule[1+3*(current_number-1)]&&current_time[0]<current_rule[3*(current_number-1)])))
							{							
								u8 clktime[2];
								for(j=0;j<current_number-1;j++)
								{
									if((current_time[1]>current_rule[1+3*j]||(current_time[1]==current_rule[1+3*j]&&current_time[0]>=current_rule[3*j]))&&(current_time[1]<current_rule[4+3*j]||(current_time[1]==current_rule[4+3*j]&&current_time[0]<current_rule[3+3*j])))	
									{		
										light=current_rule[2+3*j];
										if(light>0x00&&light<=0x64)			//规则亮度不为0
										{
											PWM_set(light*0x0030);									
											relay=1;																					
										}
										else if(light==0x00)													//规则亮度为0
										{
											relay=0;												
										}  
										clktime[0]=current_rule[3+3*j];		//记录该设定的下一个闹钟
										clktime[1]=current_rule[4+3*j];		
										WriteNbyte(0x09, clktime,2);													
									}																
								}	
							}
						}	
					}				
					//执行实时模式
					else if(IapRead(0x0400)>=0x00&&IapRead(0x0400)<=0x64)
					{
						real_flag=1;		//实时日志标志位
						if(flag_mod==0x00)
						{
							relay=0;
							light=0;
						}					
						else
						{
							PWM_set(flag_mod*0x0030);
							relay=1;
							light=(u16)flag_mod;
						}
						WriteNbyte(0x09, clk_reset, 2);
					}
					calibration_flag=1;						//校准标志位置1
					first_b7=0;   								//清除上电后第一次时钟校准标志位	
					request_flag=1;								//入网标志位
					//Usart2_Rx_Buf清零				
					for(j=0;j<Usart2_Rx_Buf[1]+3;j++)		
					{
						Usart2_Rx_Buf[j]=0;
					}
					IE2 = 0x01;			//使能串口中断		
					}					
				}		
			}
		}
}	
//设置放大倍数函数（上电放大倍数判断进行初始化）
void set_gain(void)
{
	u16 adc;
	crl=1;											//2倍放大（初始化电流小倍数放大）
	//进行一次有效值测量
	adc=RMS_Average();
	//判断电压大小，选择放大倍数
	if(adc<=0x0180)	crl=0;	//放大21倍
	else if(adc>0x0180)			crl=1;	//放大2倍
}

//实时控制命令函数，ack为1则回复，对于组播命令不要回复
void real_control(u8 control_light,u8 ack)
{
		real_flag=1;		//实时日志标志位
		//实时控制
		if(control_light>=0x00&&control_light<=0x64)
		{			
			IapErase(0x0400);//清除实时亮度
			if(control_light==0x00)	relay=0;
			else
			{	
					PWM_set(control_light*0x0030);
					relay=1;										
			}
			if(ack)		//判断是否需要回复应答
				Uart2SendStr(ACK,sizeof(ACK));
			WriteNbyte(0x09, clk_reset,2);					//关闭规则
			IapWrite(0x0400,control_light);//写入实时指令的亮度
			light=control_light;//记录当前亮度
		}
		//退出实时模式返回规则模式
		else if(control_light==0xff)
		{	
			u8 clear_1=0x02;
			IapErase(0x0400);//清除实时亮度
			ReadNbyte(0x03,current_time,2);	
			current_time[0]&=0x7f;//分
			current_time[1]&=0x3f;//时
			//有规则
			if(current_rule[0]!=0xff)
			{
				//当前时间大于等于最后一个闹钟时
				if(current_time[1]>current_rule[1+3*(current_number-1)]||(current_time[1]==current_rule[1+3*(current_number-1)]&&current_time[0]>=current_rule[3*(current_number-1)]))
				{					
					light=current_rule[2+3*(current_number-1)];
					if(light==0x00)		relay=0;
					else
						{ 
							PWM_set(light*0x0030);								
							relay=1;									
						}
					if(ack)		//判断是否需要回复应答
						Uart2SendStr(ACK,sizeof(ACK));
					WriteNbyte(0x09, clk_reset,2);										
				}
				//当前时间大于等于第一个闹钟小于最后一个闹钟时
				else if((current_time[1]>current_rule[1]||(current_time[1]==current_rule[1]&&current_time[0]>=current_rule[0]))&&(current_time[1]<current_rule[1+3*(current_number-1)]||(current_time[1]==current_rule[1+3*(current_number-1)]&&current_time[0]<current_rule[3*(current_number-1)])))
				{							
					u8 clktime[2];
					int j;
					for(j=0;j<current_number-1;j++)
					{
						if((current_time[1]>current_rule[1+3*j]||(current_time[1]==current_rule[1+3*j]&&current_time[0]>=current_rule[3*j]))&&(current_time[1]<current_rule[4+3*j]||(current_time[1]==current_rule[4+3*j]&&current_time[0]<current_rule[3+3*j])))					//当前时间大于前一个闹钟时小于后一个闹钟时
						{		
							light=current_rule[2+3*j];//记录当前亮度
							if(light>0x00&&light<=0x64)			//规则亮度不为0
							{
								PWM_set(light*0x0030);								
								relay=1;																						
							}
							else if(light==0x00)													//规则亮度为0
							{
								relay=0;
							}
							if(ack)		//判断是否需要回复应答
								Uart2SendStr(ACK,sizeof(ACK));
							clktime[0]=current_rule[3+3*j];		//设定下一个闹钟
							clktime[1]=current_rule[4+3*j];		
							WriteNbyte(0x09, clktime,2);
						}																
					}	
				}
			}
			//没有规则
			else
			{
				//没有规则全功率输出,写入00点闹钟
				PWM_set(0x12C0);									
				relay=1;
				light=0x64;
				WriteNbyte(0x09, clk_reset,2);
				if(ack)		//判断是否需要回复应答
					Uart2SendStr(ACK,sizeof(ACK));							
			}
				WriteNbyte(0x01,&clear_1,1);//使能时钟中断
			}
}


//当日规则命令函数
void today_rule(u8 size_t,u8 * rule,u8 ack)
{
	if(size_t%3==0&&size_t<=0x14)				
	{
		u16 add;
		int j=0;
		u8 clear_1=0x02;
		ReadNbyte(0x03,current_time,2);	
		current_time[0]&=0x7f;//分
		current_time[1]&=0x3f;//时
		IapErase(0x0000);	//擦除当日规则
		IapErase(0x0200);	//擦除次日规则							
		for(add=0;add<size_t;add++)					//写入当日规则分/时/亮度
		{
			IapWrite(add,rule[add]);									
		}
		for(add=0;add<size_t;add++)					//写入次日规则分/时/亮度
		{
			IapWrite(add+0x0200,rule[add]);									
		}
		for(j=0;j<size_t;j++)								//读取当日规则
		{
			current_rule[j]=IapRead(j);
		}							
		current_number=size_t/3;						//更新当日闹钟数	
		LED2=0;		
		//当前时间大于等于最后一个闹钟时
		if(current_time[1]>current_rule[1+3*(current_number-1)]||(current_time[1]==current_rule[1+3*(current_number-1)]&&current_time[0]>=current_rule[3*(current_number-1)]))
		{	
			//判断当前执行模式
			if(IapRead(0x0400)==0xff)//执行规则模式
			{
				day_clk=1;		//日志标志位
				light=current_rule[2+3*(current_number-1)];
				if(current_rule[2+3*(current_number-1)]==0x00)
				{
					relay=0;
				}
				else
				{ 
					PWM_set(light*0x0030);								
					relay=1;																			
				}
				WriteNbyte(0x09, clk_reset,2);
			}	
			else if(IapRead(0x0400)>=0x00&&IapRead(0x0400)<=0x64)//执行实时模式
			{
				WriteNbyte(0x09, clk_reset,2);	
			}
			if(ack)
				Uart2SendStr(ACK,sizeof(ACK));
		}	
		//当前时间大于第一个闹钟小于最后一个闹钟时
		else if((current_time[1]>current_rule[1]||(current_time[1]==current_rule[1]&&current_time[0]>=current_rule[0]))&&(current_time[1]<current_rule[1+3*(current_number-1)]||(current_time[1]==current_rule[1+3*(current_number-1)]&&current_time[0]<current_rule[3*(current_number-1)])))
		{							
			u8 clktime[2];//下一个规则时间
			for(j=0;j<current_number-1;j++)
			{
				if((current_time[1]>current_rule[1+3*j]||(current_time[1]==current_rule[1+3*j]&&current_time[0]>=current_rule[3*j]))&&(current_time[1]<current_rule[4+3*j]||(current_time[1]==current_rule[4+3*j]&&current_time[0]<current_rule[3+3*j])))	
				{		
					if(IapRead(0x0400)==0xff)//执行规则模式
					{
						day_clk=1;		//日志标志位
						light=current_rule[2+3*j];
						if(current_rule[2+3*j]>0x00&&current_rule[2+3*j]<=0x64)			//规则亮度不为0
						{
							PWM_set(light*0x0030);									
							relay=1;																					
						}
						else if(current_rule[2+3*j]==0x00)													//规则亮度为0
						{
							relay=0;												
						}
						clktime[0]=current_rule[3+3*j];		//记录该设定的下一个闹钟
						clktime[1]=current_rule[4+3*j];		
						WriteNbyte(0x09, clktime,2);
					}
					//执行实时模式
					else if(IapRead(0x0400)>=0x00&&IapRead(0x0400)<=0x64)
					{
						WriteNbyte(0x09, clk_reset,2);
					}
					if(ack)
						Uart2SendStr(ACK,sizeof(ACK));	
				}																
			}	
		}
		WriteNbyte(0x01, &clear_1,1);	//使能闹钟中断
	}
	else if(ack)
		Uart2SendStr(NOACK,sizeof(NOACK));
}

//次日规则命令函数，传入数据长度，规则以及是否应答
void tomorrow_rule(u8 size_t,u8 * rule,u8 ack)
{
	u8 clear_1=0x02;
	//只接收6个时间段
	if(size_t%3==0&&size_t<0x14)				
	{
		u16 add;								
		IapErase(0x0200);//清除次日规则																							
		for(add=0;add<size_t;add++)					//写入分/时/亮度(下一天的规则写在第二页)
		{
			IapWrite((add+0x0200),rule[add]);										
		}
		//当前没规则
		if(IapRead(0x0000)==0xff)
		{
			WriteNbyte(0x09, clk_reset,2);
			WriteNbyte(0x01, &clear_1,1);	//使能闹钟中断
		}
		if(ack)
			Uart2SendStr(ACK,sizeof(ACK));
	}
	else if(ack)
		Uart2SendStr(NOACK,sizeof(NOACK));
}