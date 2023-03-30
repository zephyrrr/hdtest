#ifndef _TIM_H
#define _TIM_H
#include <STC8.h>
extern bit adc_flag;
extern u16 voldata[100];
//extern u16 sum;
void Timer0Init(void);
u16 RMS_count(u16 *vol,u16 sizet);
u16 RMS_ONE(void);
u16 RMS_Average(void);
#endif
