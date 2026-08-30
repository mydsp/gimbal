#ifndef __GIMBAL_H
#define __GIMBAL_H

#include "main.h"

typedef enum { MODE_POT, MODE_ATTITUDE, MODE_VOFA } GimbalMode;

typedef struct {
  uint8_t  pan_deg;          /* 0~180 */
  uint8_t  tilt_deg;         /* 0~180 */
  uint16_t pan_pwm;          /* 实际输出脉宽 */
  uint16_t tilt_pwm;
  GimbalMode mode;
} GimbalState;

void Gimbal_Init(void);
void Gimbal_SetMode(GimbalMode mode);
void Gimbal_PotControl(void);
void Gimbal_AttitudeControl(void);
void Gimbal_VOFAControl(uint8_t pan, uint8_t tilt);
void Gimbal_GetState(GimbalState *s);

#endif