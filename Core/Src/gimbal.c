#include "gimbal.h"
#include "pwm.h"
#include "adc.h"
#include "mpu6050.h"

static GimbalState state = {90, 90, 1500, 1500, MODE_POT};

void Gimbal_Init(void)
{
  Servo_SetAngle(90, 90);
}

void Gimbal_SetMode(GimbalMode mode)
{
  state.mode = mode;
}

void Gimbal_PotControl(void)
{
  uint16_t v0 = ADC_Read(ADC_CHANNEL_0);   /* PA0 */
  uint16_t v1 = ADC_Read(ADC_CHANNEL_1);   /* PA1 */
  uint8_t pan  = (uint8_t)ADC_ToAngle(v0);
  uint8_t tilt = (uint8_t)ADC_ToAngle(v1);
  Servo_SetAngle(pan, tilt);
  Servo_GetState(&state.pan_deg, &state.tilt_deg, &state.pan_pwm, &state.tilt_pwm);
}

void Gimbal_AttitudeControl(void)
{
  MPU_Attitude att;
  MPU_GetAttitude(&att);

  /* 取反：面包板倾斜方向与云台跟随方向相反 */
  uint8_t pan  = (uint8_t)(90 - att.roll);
  uint8_t tilt = (uint8_t)(90 + att.pitch);
  if (pan  > 180) pan  = 180;
  if (tilt > 180) tilt = 180;

  Servo_SetAngle(pan, tilt);
  Servo_GetState(&state.pan_deg, &state.tilt_deg, &state.pan_pwm, &state.tilt_pwm);
}

void Gimbal_VOFAControl(uint8_t pan, uint8_t tilt)
{
  Servo_SetAngle(pan, tilt);
  Servo_GetState(&state.pan_deg, &state.tilt_deg, &state.pan_pwm, &state.tilt_pwm);
}

void Gimbal_GetState(GimbalState *s)
{
  if (s) *s = state;
}