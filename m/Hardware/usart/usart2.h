#ifndef _USART2_H
#define _USART2_H

#include <STC8.h>

extern u8 Usart2_Rx_Buf[20];
extern u8 Usart2_Rx_Sum;
extern u8 ACK[4];
extern u8 NOACK[4];
extern bit CRC_OK;
void Uart2_Init_9600();
void Uart2_Init_115200();
void Uart2Send(u8 dat);
void Uart2SendStr(u8 *p,u8 i);
void putchar(char c);
void LoRa_Config();

#endif