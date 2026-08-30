#ifndef __ADC_H
#define __ADC_H

#include "main.h"

void    ADC_Init(void);
uint16_t ADC_Read(uint32_t ch);
float   ADC_ToAngle(uint16_t raw);

#endif