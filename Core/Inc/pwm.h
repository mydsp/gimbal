#ifndef __PWM_H
#define __PWM_H

#include "main.h"

void        PWM_Init(void);
void        Servo_SetAngle(uint8_t pan, uint8_t tilt);
uint16_t   Servo_GetPWM(uint8_t ch);
void        Servo_GetState(uint8_t *pan, uint8_t *tilt, uint16_t *pwm_pan, uint16_t *pwm_tilt);

#endif