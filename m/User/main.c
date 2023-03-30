/*****************************************************************
��Ŀ���ƣ��ǻ�����Lora2.0����ˮ������
���䣺0x0000 �������
			0x0200 ���չ���
			0x0400 ʵʱ��������
			0x0600 ���
			0x0800 lora���ñ�־λ
���ߣ�������
�޸����ڣ�2022��10��10��

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

const u16 OPEN=0x005A;   //����ʱ�Ŵ�21��������0x000D���ң����20w���ϵĵƾ�,������(�ݶ���ֵ�����ڲ�ͬ���ʵĵƻ������)
u8 request[5];													//��������ָ��
u8 no_open[4]={0xB2,0x01,0x65,0x18};		//�޷����ƹ���
u8 right[4];														//����
u8 no_acheive[4]={0xB2,0x01,0x67,0x1A};	//δ�ﵽָ������	��δ�����صƣ�
u8 clk_reset[2]={0x00,0x00};						//�������ʱ��
u8 cal_ok[4]={0xB2,0x01,0xFE,0xB1};			//���ʱ��У׼
u8 real_ok[5];    											//��־��־
u8 time_set[6];													//ʱ��У׼
u8 clk_time[2]=0;
u8 current_rule[20];				//����ʱ�����ÿ��24����º��¼��
u8 next_rule[20];
u8 current_number;					//������������ÿ��24����£�
u8 current_time[2];				  //��ǰʱ��	
u16 light;									//�����л�ʱ��¼����
u8 next_clk[2];
bit calibration_flag;       //ʱ��У׼��־λ
bit group_flag;							//�鲥��־λ
bit day_clk;								//������־��־
bit real_flag;							//ʵʱ��־��־
bit first_b7;								//����ʱ��У׼λ
bit B2_flag;								//�յ�B2��־λ	
bit res_flag;								//�鲥�����־λ
bit request_flag;

int main(void)
{	
	
	u8 i;
	u8 TIME[2];
	u16 addr;
	u8 seed;		//����ʱ������
	u16 adc;
	u16 j;
	u8 power;		//���뿪��
	u16 powerdata=0;//��ѹ��Чֵ
	CRC_OK=0;
	sysclk_init();
	gpioconfig();
	PWM_init();	
	ADC11_init();	
	Uart2_Init_115200();
	IT0=1;			//ʹ��INT0�½���
	EX0=1;			//ʹ��INT0�ж�
	EA = 1;	
	addr=0;	
	relay=1;										//�̵����պ�
	request_flag=0;							//���������־λ
  calibration_flag=0;					//ʱ��У׼��־λ
	group_flag=0;
	INT_flag=0;			            //RTC �жϱ�־λ
	first_b7=1;									//����У׼��־λ
	res_flag=1;									//Loraģ�鸴λ��־
	B2_flag=0;
	crl=1;											//2���Ŵ󣨳�ʼ������С�����Ŵ�
	adc_flag=0;									//AD���ڲɼ�������־
	light=0x64;
	day_clk=0;
	real_flag=0;
	CRC_OK=0;
	power=((P0&0x10)>>4)*8+((P0&0x20)>>5)*4+((P0&0x40)>>6)*2+((P0&0x80)>>7);//4λ���루���ܴ�����
	rest=1;
	//��յ�ǰ��������
	for(j=0;j<20;j++)
	{
		current_rule[j]=0xff;
	}
	seed=ADC11_Get();						//��ȡ�������
	if(seed<0x64)
		seed+=0x64;
	ADC_init();	
	set_gain();	//ѡ��Ŵ�����2����21����
	Delay1ms(seed*2);	//�����ʱ
	RTC_init();
	//IICͨ�Ų���
//	ReadNbyte(0x03,current_time,2);	
//	current_time[0]&=0x7f;//��
//	current_time[1]&=0x3f;//ʱ	
//	Uart2SendStr(current_time,sizeof(current_time));
	powerdata=RMS_Average();
	//printf("%4x",powerdata);
	//�жϷŴ���
	if(crl)//�Ŵ�2��
	{
		LED2=0;
		powerdata/=2;
	}
	else if(!crl)
		powerdata/=21;
	//��������ָ��
	request[0]=0xBA;
	request[1]=0x02;
	request[2]=powerdata>>8;
	request[3]=(u8)powerdata;
	request[4]=request[0]+request[1]+request[2]+request[3];
	if(IapRead(0x0800)==0xff)
	{
		LoRa_Config();			//Loraģ������	
		LED3=1;
		IapWrite(0x0800,0x01);
	}
	WDT_CONTR=0x27;                  //�������Ź���8s
	while(1) 
		{		
			WDT_CONTR|=0x10;					//�忴�Ź�������ϵͳ��λ
			//�ɹ�����
			if(request_flag)
			{ 		
				LED2=1;					//LED2--�������ָʾ��
				LED1=1;					//LED1--����ʱ����л�ָʾ��
				LED0=~LED0;			//��������ָʾ��
				Delay500ms();
				ReadNbyte(0x03,current_time,2);	//�鿴��ǰʱ��
				current_time[0]&=0x7f;//��
				current_time[1]&=0x3f;//ʱ
				//��2min��λLoraģ��
				if(current_time[0]%2==0&&res_flag)
				{				
						//��λģ��
						rest=0;
						Delay30us();
						rest=1;
						res_flag=0;					
				}	
				if(current_time[0]%2!=0&&!res_flag)
				{
					res_flag=1;
				}															
				//�Ϸ�����
				if(CRC_OK)//���У����ȷ
				{		
						CRC_OK=0;					//��У����ȷ��־λ					
						if(Usart2_Rx_Buf[0]==0xB2)						//�����豸״̬�ϴ�
						{											
							u8 c=0;	
							B2_flag=1;			//�յ�B2��־λ				
							ReadNbyte(0x03,current_time,2);	
							current_time[0]&=0x7f;//��
							current_time[1]&=0x3f;//ʱ
							WDT_CONTR|=0x10;					//�忴�Ź�������ϵͳ��λ
							//�鲥�����־
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
							//ʱ��У׼��־
							else if(calibration_flag)
							{							
								Uart2SendStr(cal_ok,sizeof(cal_ok));
								calibration_flag=0;
							}
							//������־��־
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
							//ʵʱ��־��־
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
							//��ǰ����ִ�й���ģʽ
							else if(IapRead(0x0400)==0xff)							
							{
								//�й���
								if(current_rule[0]!=0xff)
								{
									right[0]=0xB2;
									right[1]=0x01;
									right[2]=light;
									right[3]=right[0]+right[1]+right[2];
									adc=RMS_Average();	//�ɼ���ѹ��Чֵ
									//�������һ������ʱ��ʱ
									if(current_time[1]>current_rule[1+3*(current_number-1)]||(current_time[1]==current_rule[1+3*(current_number-1)]&&current_time[0]>current_rule[3*(current_number-1)]))
									{														
										if(current_rule[2+3*(current_number-1)]>0)									//����������Ȳ�Ϊ0
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
									//���ڵ�һ������С�����һ������ʱ��ʱ
									else if((current_time[1]>current_rule[1]||(current_time[1]==current_rule[1]&&current_time[0]>current_rule[0]))&&(current_time[1]<current_rule[1+3*(current_number-1)]||(current_time[1]==current_rule[1+3*(current_number-1)]&&current_time[0]<current_rule[3*(current_number-1)])))
									{						
										for(c=0;c<current_number-1;c++)
										{
											if((current_time[1]>current_rule[1+3*c]||(current_time[1]==current_rule[1+3*c]&&current_time[0]>current_rule[3*c]))&&(current_time[1]<current_rule[4+3*c]||(current_time[1]==current_rule[4+3*c]&&current_time[0]<current_rule[3+3*c])))					//��ǰʱ�����ǰһ������ʱС�ں�һ������ʱ
											{								
												if(current_rule[2+3*c]>0x00&&current_rule[2+3*c]<=0x64)			//�������Ȳ�Ϊ0
												{
													if(adc>OPEN)
														Uart2SendStr(right,sizeof(right));
													else
														Uart2SendStr(no_open,sizeof(no_open));
												}
												else if(current_rule[2+3*c]==0x00)													//��������Ϊ0
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
								//û�й���
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
							//��ǰ����ִ��ʵʱģʽ
							else if(IapRead(0x0400)!=0xff)
							{
								light=IapRead(0x0400);
								right[0]=0xB2;
								right[1]=0x01;
								right[2]=light;
								right[3]=right[0]+right[1]+right[2];
								adc=RMS_Average();	//�ɼ���ѹ��Чֵ
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
							//Usart2_Rx_Buf����
							for(j=0;j<Usart2_Rx_Buf[1]+3;j++)		
							{
								Usart2_Rx_Buf[j]=0;
							}
						}									
						else if(Usart2_Rx_Buf[0]==0xB3)						//���ص�
						{
							//ʵʱ���ص��������ʵʱģʽ						
							if((Usart2_Rx_Buf[2]>=0x00&&Usart2_Rx_Buf[2]<=0x64)||Usart2_Rx_Buf[2]==0xff)
							{
								real_control(Usart2_Rx_Buf[2],1);
							}
							else 
								Uart2SendStr(NOACK,sizeof(NOACK));
							//Usart2_Rx_Buf����				
							for(j=0;j<Usart2_Rx_Buf[1]+3;j++)		
							{
								Usart2_Rx_Buf[j]=0;
							}
							IE2 = 0x01;			//ʹ�ܴ����ж�						
						}
						else if(Usart2_Rx_Buf[0]==0xB5)						//������գ���/ʱ/���ȣ���һ��Ĺ���
						{
							u8 rule[20];
							for(j=0;i<Usart2_Rx_Buf[1];j++)
							{
								rule[j]=Usart2_Rx_Buf[2+j];
							}
							tomorrow_rule(Usart2_Rx_Buf[1],rule,1);
							//Usart2_Rx_Buf����				
							for(j=0;j<Usart2_Rx_Buf[1]+3;j++)		
							{
								Usart2_Rx_Buf[j]=0;
							}
							IE2 = 0x01;			//ʹ�ܴ����ж�
						}
						else if(Usart2_Rx_Buf[0]==0xB6)						//�����޸ģ��޸ĵ������
						{
							u8 rule[20];
							for(j=0;j<Usart2_Rx_Buf[1];j++)
							{
								rule[j]=Usart2_Rx_Buf[2+j];
							}
							today_rule(Usart2_Rx_Buf[1],rule,1);
							//Usart2_Rx_Buf����				
							for(j=0;j<Usart2_Rx_Buf[1]+3;j++)		
							{
								Usart2_Rx_Buf[j]=0;
							}
							IE2 = 0x01;			//ʹ�ܴ����ж�
						}
						else if(Usart2_Rx_Buf[0]==0xB7)						//ʱ��У׼
						{
							if(Usart2_Rx_Buf[1]==0x08)
							{
								//д��У׼ʱ��
								for(i=0;i<6;i++)
								{						
									time_set[i]=Usart2_Rx_Buf[2+i];								
								}
								WriteNbyte(0x02, time_set, 7);
								calibration_flag=1;						//У׼��־λ��1
								LED3=0;
								Delay500ms();	
								LED3=1;									
							}
							else
								Uart2SendStr(NOACK,sizeof(NOACK));							
							//Usart2_Rx_Buf����				
							for(j=0;j<Usart2_Rx_Buf[1]+3;j++)		
							{
								Usart2_Rx_Buf[j]=0;
							}
							IE2 = 0x01;			//ʹ�ܴ����ж�						
						}																					
						else if(Usart2_Rx_Buf[0]==0xB8)						//��������
						{
							//��¼���							
							IapErase(0x0600);//�������
							if(Usart2_Rx_Buf[2]!=0xff)
								IapWrite(0x0600,Usart2_Rx_Buf[2]);//д�����
							Uart2SendStr(ACK,sizeof(ACK));
							//Usart2_Rx_Buf����				
							for(j=0;j<Usart2_Rx_Buf[1]+3;j++)		
							{
								Usart2_Rx_Buf[j]=0;
							}
							IE2 = 0x01;			//ʹ�ܴ����ж�	
						}
						else if(Usart2_Rx_Buf[0]==0xBB)						//�鲥ʵʱ��������
						{
							if(Usart2_Rx_Buf[2]==IapRead(0x0600))		//�鿴����Ƿ�ƥ��		
							{
								if((Usart2_Rx_Buf[3]>=0x00&&Usart2_Rx_Buf[3]<=0x64)||Usart2_Rx_Buf[3]==0xff)
								{
									real_control(Usart2_Rx_Buf[3],0);
								}
//								else 
//									Uart2SendStr(NOACK,sizeof(NOACK));
							}
							//Usart2_Rx_Buf����				
							for(j=0;j<Usart2_Rx_Buf[1]+3;j++)		
							{
								Usart2_Rx_Buf[j]=0;
							}
							IE2 = 0x01;			//ʹ�ܴ����ж�	
						}							
						else if(Usart2_Rx_Buf[0]==0xBD)						//�鲥���չ�������
						{
							if(Usart2_Rx_Buf[2]==IapRead(0x0600))		//�鿴����Ƿ�ƥ��		
							{
								u8 rule[20];
								for(j=0;j<Usart2_Rx_Buf[1]-1;j++)//��ȡ����
								{
									rule[j]=Usart2_Rx_Buf[3+j];
								}
								tomorrow_rule(Usart2_Rx_Buf[1]-1,rule,0);
								group_flag=1;		//�鲥��־λ
								day_clk=0;
							}
							//Usart2_Rx_Buf����				
							for(j=0;j<Usart2_Rx_Buf[1]+3;j++)		
							{
								Usart2_Rx_Buf[j]=0;
							}
							IE2 = 0x01;			//ʹ�ܴ����ж�
						}
						else if(Usart2_Rx_Buf[0]==0xBE)						//�鲥���չ�������
						{
							if(Usart2_Rx_Buf[2]==IapRead(0x0600))		//�鿴����Ƿ�ƥ��		
							{
								u8 rule[20];
								for(j=0;j<Usart2_Rx_Buf[1]-1;j++)//��ȡ����
								{
									rule[j]=Usart2_Rx_Buf[3+j];
								}
								today_rule(Usart2_Rx_Buf[1]-1,rule,0);//����Ӧ�𣬵��´���ѯ�ϱ�
								group_flag=1;		//�鲥��־λ
								day_clk=0;
							}
							//Usart2_Rx_Buf����				
							for(j=0;j<Usart2_Rx_Buf[1]+3;j++)		
							{
								Usart2_Rx_Buf[j]=0;
							}
							IE2 = 0x01;			//ʹ�ܴ����ж�
						}
						else
						{
							//Usart2_Rx_Buf����				
							for(j=0;j<Usart2_Rx_Buf[1]+3;j++)		
							{
								Usart2_Rx_Buf[j]=0;
							}
						}
				}
				//�ж�RTC��ʱ�ж�
				if(INT_flag)
				{
					u8 clear_1=0x02;
					//�ж��Ƿ����趨������ ���鿴��ǰʱ�䣩
					ReadNbyte(0x03,TIME,2);
					Delay100ms();
					TIME[0]&=0x7f;//��
					TIME[1]&=0x3f;//ʱ
					INT_flag=0;
					Delay500ms();
					WriteNbyte(0x01,&clear_1,1); 								//���RTC��ʱ�жϱ�־													
					//�����24������ӣ��������һ�������
					if(TIME[0]==0x00&&TIME[1]==0x00)
					{		
						LED2=0;	
						Delay500ms();						
						//�滻����
						IapErase(0x0000);													//������һ��Ĺ���	
						for(addr=0;addr<20;addr++)					//д�뵱������/ʱ/����(����Ĺ���д�ڵ�һҳ��
						{							
							next_rule[addr]=IapRead(0x0200+addr);
						}						
						for(addr=0;addr<20;addr++)					//д�뵱������/ʱ/����(����Ĺ���д�ڵ�һҳ��
						{							
							IapWrite(addr,next_rule[addr]);
						}
						for(addr=0;addr<20;addr++)		
						{
							current_rule[addr]=IapRead(addr);					//���µ�ǰ���򣬷�/ʱ/����
						}
						addr=0;
						while(current_rule[addr]!=0xff)
						{
							addr++;
						}	
						current_number=addr/3;													//��¼��ǰ���Ӹ���	
						//Uart2Send(current_number);						
						//��ǰΪ����ģʽ
						if(IapRead(0x0400)==0xff)
						{
							day_clk=1;		//��־��־λ
							//�й���
							if(current_rule[0]!=0xff)
							{
								//ִ�е����һ�����ӣ�0�㣩
								light=current_rule[2];
								if(light==0) relay=0;
								else
								{				
									PWM_set(light*0x0030);							
									relay=1;
								}
								if(current_number>=2)
								{
									//���õ���ڶ�������					
								clk_time[0]=current_rule[3];
								clk_time[1]=current_rule[4];					
								WriteNbyte(0x09,clk_time,2);
								//Uart2SendStr(clk_time,sizeof(clk_time));
								}
								else
									WriteNbyte(0x09,clk_reset,2);
							}
							//û�й���
							else if(current_rule[0]==0xff)
							{
								light=0x64;
								PWM_set(0x12C0);							
								relay=1;							
							}							
						}	
						//��ǰΪʵʱģʽ
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
					//�������24�������
					else
					{	
						day_clk=1;		//��־��־λ
						LED1=0;
						Delay500ms();	
						for(j=1;j<current_number;j++)
						{
							if(TIME[0]==current_rule[3*j]&&TIME[1]==current_rule[1+3*j])
							{
								light=current_rule[2+3*j];
								if(j==current_number-1)//���һ��ʱ���
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
									//������һ������	
									clk_time[0]=current_rule[3+3*j];
									clk_time[1]=current_rule[4+3*j];														
									WriteNbyte(0x09,clk_time,2);
									//Uart2SendStr(clk_time,sizeof(clk_time));
								}
							}
						}	
						//WriteNbyte(0x01,&tmp[1],1);
					}						
					WriteNbyte(0x01,&clear_1,1); 								//���RTC��ʱ�жϱ�־	
					IE2=0x01;
				}				
			}
			//δ�����ɹ���LED0�������������������������Ƴ���
			else
			{								
				//����������û�й�����ʵʱָ�
				LED0=0;				
				Uart2SendStr(request,sizeof(request));				//�ȴ�����
				Delay1ms(seed);
				WDT_CONTR|=0x10;					//�忴�Ź�������ϵͳ��λ
				Delay1ms(seed);
				WDT_CONTR|=0x10;					//�忴�Ź�������ϵͳ��λ
				//�ж��Ƿ����ϵ���һ��ʱ��У׼����Ϊ������
				if(CRC_OK)
				{
					CRC_OK=0;
					if(first_b7&&Usart2_Rx_Buf[0]==0xB7&&Usart2_Rx_Buf[1]==0x08)
					{
					u8 h=0;
					u8 flag_mod;//�鿴�Ƿ���ʵʱģʽ
					//д��У׼ʱ��
					for(i=0;i<6;i++)
					{						
						time_set[i]=Usart2_Rx_Buf[2+i];								
					}
					WriteNbyte(0x02, time_set, 7);
					//��EEPROM�ж�ȡ��ǰ���������	
					for(j=0;j<20;j++)
					{
						current_rule[j]=IapRead(j);
					}
					while(current_rule[h]!=0xff)		 
					{										
						h++;									
					}
					//����IICͨ���Ƿ�����
//					ReadNbyte(0x03,current_time,2);	
//					current_time[0]&=0x7f;//��
//					current_time[1]&=0x3f;//ʱ	
//					Uart2SendStr(current_time,sizeof(current_time));
					current_number=h/3;				//��¼��������
					flag_mod=IapRead(0x0400);	//�鿴�Ƿ���ʵʱģʽ
					//ִ�й���ģʽ
					if(IapRead(0x0400)==0xff)
					{
						day_clk=1;		//������־��־λ																				
						//������û�й���Ƴ���(%100)
						if(current_rule[0]==0xff)
						{
							relay=1;
							light=0x64;
							WriteNbyte(0x09,clk_reset,2);
						}	
						//�������й��򰴹�����
						else if(current_rule[0]!=0xff)
						{
							//�鿴��ǰʱ��
							ReadNbyte(0x03,current_time,2);	
							current_time[0]&=0x7f;//��
							current_time[1]&=0x3f;//ʱ	
							//��ǰʱ����ڵ������һ������ʱ
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
							//��ǰʱ�������ڵ�һ������С�����һ������ʱ
							else if((current_time[1]>current_rule[1]||(current_time[1]==current_rule[1]&&current_time[0]>=current_rule[0]))&&(current_time[1]<current_rule[1+3*(current_number-1)]||(current_time[1]==current_rule[1+3*(current_number-1)]&&current_time[0]<current_rule[3*(current_number-1)])))
							{							
								u8 clktime[2];
								for(j=0;j<current_number-1;j++)
								{
									if((current_time[1]>current_rule[1+3*j]||(current_time[1]==current_rule[1+3*j]&&current_time[0]>=current_rule[3*j]))&&(current_time[1]<current_rule[4+3*j]||(current_time[1]==current_rule[4+3*j]&&current_time[0]<current_rule[3+3*j])))	
									{		
										light=current_rule[2+3*j];
										if(light>0x00&&light<=0x64)			//�������Ȳ�Ϊ0
										{
											PWM_set(light*0x0030);									
											relay=1;																					
										}
										else if(light==0x00)													//��������Ϊ0
										{
											relay=0;												
										}  
										clktime[0]=current_rule[3+3*j];		//��¼���趨����һ������
										clktime[1]=current_rule[4+3*j];		
										WriteNbyte(0x09, clktime,2);													
									}																
								}	
							}
						}	
					}				
					//ִ��ʵʱģʽ
					else if(IapRead(0x0400)>=0x00&&IapRead(0x0400)<=0x64)
					{
						real_flag=1;		//ʵʱ��־��־λ
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
					calibration_flag=1;						//У׼��־λ��1
					first_b7=0;   								//����ϵ���һ��ʱ��У׼��־λ	
					request_flag=1;								//������־λ
					//Usart2_Rx_Buf����				
					for(j=0;j<Usart2_Rx_Buf[1]+3;j++)		
					{
						Usart2_Rx_Buf[j]=0;
					}
					IE2 = 0x01;			//ʹ�ܴ����ж�		
					}					
				}		
			}
		}
}	
//���÷Ŵ����������ϵ�Ŵ����жϽ��г�ʼ����
void set_gain(void)
{
	u16 adc;
	crl=1;											//2���Ŵ󣨳�ʼ������С�����Ŵ�
	//����һ����Чֵ����
	adc=RMS_Average();
	//�жϵ�ѹ��С��ѡ��Ŵ���
	if(adc<=0x0180)	crl=0;	//�Ŵ�21��
	else if(adc>0x0180)			crl=1;	//�Ŵ�2��
}

//ʵʱ�����������ackΪ1��ظ��������鲥���Ҫ�ظ�
void real_control(u8 control_light,u8 ack)
{
		real_flag=1;		//ʵʱ��־��־λ
		//ʵʱ����
		if(control_light>=0x00&&control_light<=0x64)
		{			
			IapErase(0x0400);//���ʵʱ����
			if(control_light==0x00)	relay=0;
			else
			{	
					PWM_set(control_light*0x0030);
					relay=1;										
			}
			if(ack)		//�ж��Ƿ���Ҫ�ظ�Ӧ��
				Uart2SendStr(ACK,sizeof(ACK));
			WriteNbyte(0x09, clk_reset,2);					//�رչ���
			IapWrite(0x0400,control_light);//д��ʵʱָ�������
			light=control_light;//��¼��ǰ����
		}
		//�˳�ʵʱģʽ���ع���ģʽ
		else if(control_light==0xff)
		{	
			u8 clear_1=0x02;
			IapErase(0x0400);//���ʵʱ����
			ReadNbyte(0x03,current_time,2);	
			current_time[0]&=0x7f;//��
			current_time[1]&=0x3f;//ʱ
			//�й���
			if(current_rule[0]!=0xff)
			{
				//��ǰʱ����ڵ������һ������ʱ
				if(current_time[1]>current_rule[1+3*(current_number-1)]||(current_time[1]==current_rule[1+3*(current_number-1)]&&current_time[0]>=current_rule[3*(current_number-1)]))
				{					
					light=current_rule[2+3*(current_number-1)];
					if(light==0x00)		relay=0;
					else
						{ 
							PWM_set(light*0x0030);								
							relay=1;									
						}
					if(ack)		//�ж��Ƿ���Ҫ�ظ�Ӧ��
						Uart2SendStr(ACK,sizeof(ACK));
					WriteNbyte(0x09, clk_reset,2);										
				}
				//��ǰʱ����ڵ��ڵ�һ������С�����һ������ʱ
				else if((current_time[1]>current_rule[1]||(current_time[1]==current_rule[1]&&current_time[0]>=current_rule[0]))&&(current_time[1]<current_rule[1+3*(current_number-1)]||(current_time[1]==current_rule[1+3*(current_number-1)]&&current_time[0]<current_rule[3*(current_number-1)])))
				{							
					u8 clktime[2];
					int j;
					for(j=0;j<current_number-1;j++)
					{
						if((current_time[1]>current_rule[1+3*j]||(current_time[1]==current_rule[1+3*j]&&current_time[0]>=current_rule[3*j]))&&(current_time[1]<current_rule[4+3*j]||(current_time[1]==current_rule[4+3*j]&&current_time[0]<current_rule[3+3*j])))					//��ǰʱ�����ǰһ������ʱС�ں�һ������ʱ
						{		
							light=current_rule[2+3*j];//��¼��ǰ����
							if(light>0x00&&light<=0x64)			//�������Ȳ�Ϊ0
							{
								PWM_set(light*0x0030);								
								relay=1;																						
							}
							else if(light==0x00)													//��������Ϊ0
							{
								relay=0;
							}
							if(ack)		//�ж��Ƿ���Ҫ�ظ�Ӧ��
								Uart2SendStr(ACK,sizeof(ACK));
							clktime[0]=current_rule[3+3*j];		//�趨��һ������
							clktime[1]=current_rule[4+3*j];		
							WriteNbyte(0x09, clktime,2);
						}																
					}	
				}
			}
			//û�й���
			else
			{
				//û�й���ȫ�������,д��00������
				PWM_set(0x12C0);									
				relay=1;
				light=0x64;
				WriteNbyte(0x09, clk_reset,2);
				if(ack)		//�ж��Ƿ���Ҫ�ظ�Ӧ��
					Uart2SendStr(ACK,sizeof(ACK));							
			}
				WriteNbyte(0x01,&clear_1,1);//ʹ��ʱ���ж�
			}
}


//���չ��������
void today_rule(u8 size_t,u8 * rule,u8 ack)
{
	if(size_t%3==0&&size_t<=0x14)				
	{
		u16 add;
		int j=0;
		u8 clear_1=0x02;
		ReadNbyte(0x03,current_time,2);	
		current_time[0]&=0x7f;//��
		current_time[1]&=0x3f;//ʱ
		IapErase(0x0000);	//�������չ���
		IapErase(0x0200);	//�������չ���							
		for(add=0;add<size_t;add++)					//д�뵱�չ����/ʱ/����
		{
			IapWrite(add,rule[add]);									
		}
		for(add=0;add<size_t;add++)					//д����չ����/ʱ/����
		{
			IapWrite(add+0x0200,rule[add]);									
		}
		for(j=0;j<size_t;j++)								//��ȡ���չ���
		{
			current_rule[j]=IapRead(j);
		}							
		current_number=size_t/3;						//���µ���������	
		LED2=0;		
		//��ǰʱ����ڵ������һ������ʱ
		if(current_time[1]>current_rule[1+3*(current_number-1)]||(current_time[1]==current_rule[1+3*(current_number-1)]&&current_time[0]>=current_rule[3*(current_number-1)]))
		{	
			//�жϵ�ǰִ��ģʽ
			if(IapRead(0x0400)==0xff)//ִ�й���ģʽ
			{
				day_clk=1;		//��־��־λ
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
			else if(IapRead(0x0400)>=0x00&&IapRead(0x0400)<=0x64)//ִ��ʵʱģʽ
			{
				WriteNbyte(0x09, clk_reset,2);	
			}
			if(ack)
				Uart2SendStr(ACK,sizeof(ACK));
		}	
		//��ǰʱ����ڵ�һ������С�����һ������ʱ
		else if((current_time[1]>current_rule[1]||(current_time[1]==current_rule[1]&&current_time[0]>=current_rule[0]))&&(current_time[1]<current_rule[1+3*(current_number-1)]||(current_time[1]==current_rule[1+3*(current_number-1)]&&current_time[0]<current_rule[3*(current_number-1)])))
		{							
			u8 clktime[2];//��һ������ʱ��
			for(j=0;j<current_number-1;j++)
			{
				if((current_time[1]>current_rule[1+3*j]||(current_time[1]==current_rule[1+3*j]&&current_time[0]>=current_rule[3*j]))&&(current_time[1]<current_rule[4+3*j]||(current_time[1]==current_rule[4+3*j]&&current_time[0]<current_rule[3+3*j])))	
				{		
					if(IapRead(0x0400)==0xff)//ִ�й���ģʽ
					{
						day_clk=1;		//��־��־λ
						light=current_rule[2+3*j];
						if(current_rule[2+3*j]>0x00&&current_rule[2+3*j]<=0x64)			//�������Ȳ�Ϊ0
						{
							PWM_set(light*0x0030);									
							relay=1;																					
						}
						else if(current_rule[2+3*j]==0x00)													//��������Ϊ0
						{
							relay=0;												
						}
						clktime[0]=current_rule[3+3*j];		//��¼���趨����һ������
						clktime[1]=current_rule[4+3*j];		
						WriteNbyte(0x09, clktime,2);
					}
					//ִ��ʵʱģʽ
					else if(IapRead(0x0400)>=0x00&&IapRead(0x0400)<=0x64)
					{
						WriteNbyte(0x09, clk_reset,2);
					}
					if(ack)
						Uart2SendStr(ACK,sizeof(ACK));	
				}																
			}	
		}
		WriteNbyte(0x01, &clear_1,1);	//ʹ�������ж�
	}
	else if(ack)
		Uart2SendStr(NOACK,sizeof(NOACK));
}

//���չ�����������������ݳ��ȣ������Լ��Ƿ�Ӧ��
void tomorrow_rule(u8 size_t,u8 * rule,u8 ack)
{
	u8 clear_1=0x02;
	//ֻ����6��ʱ���
	if(size_t%3==0&&size_t<0x14)				
	{
		u16 add;								
		IapErase(0x0200);//������չ���																							
		for(add=0;add<size_t;add++)					//д���/ʱ/����(��һ��Ĺ���д�ڵڶ�ҳ)
		{
			IapWrite((add+0x0200),rule[add]);										
		}
		//��ǰû����
		if(IapRead(0x0000)==0xff)
		{
			WriteNbyte(0x09, clk_reset,2);
			WriteNbyte(0x01, &clear_1,1);	//ʹ�������ж�
		}
		if(ack)
			Uart2SendStr(ACK,sizeof(ACK));
	}
	else if(ack)
		Uart2SendStr(NOACK,sizeof(NOACK));
}