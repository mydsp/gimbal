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

  /* 取反：面包板倾斜方向与云台跟随方向相反。
   * 关键：先用有符号中间量算好角度，再钳位到 [0,180]，最后才转 uint8。
   * 若先转 uint8 再钳位，负数会回绕成 246 并被钳到 180（而非 0），两端必现错误。 */
  int pan_raw  = 90 - (int)att.roll;
  int tilt_raw = 90 + (int)att.pitch;
  if (pan_raw  < 0)   pan_raw  = 0;
  if (pan_raw  > 180) pan_raw  = 180;
  if (tilt_raw < 0)   tilt_raw = 0;
  if (tilt_raw > 180) tilt_raw = 180;

  Servo_SetAngle((uint8_t)pan_raw, (uint8_t)tilt_raw);
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