#include "pwm.h"

static TIM_HandleTypeDef htim3;

void PWM_Init(void)
{
  __HAL_RCC_TIM3_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitTypeDef g = {0};
  g.Pin = SERVO_PAN_PIN | SERVO_TILT_PIN;
  g.Mode = GPIO_MODE_AF_PP;
  g.Pull = GPIO_NOPULL;
  g.Speed = GPIO_SPEED_FREQ_HIGH;
  g.Alternate = GPIO_AF2_TIM3;
  HAL_GPIO_Init(SERVO_PORT, &g);

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 84 - 1;          /* 84MHz / 84 = 1MHz */
  htim3.Init.Period = 20000 - 1;          /* 1MHz / 20000 = 50Hz */
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_PWM_Init(&htim3);

  TIM_OC_InitTypeDef oc = {0};
  oc.OCMode = TIM_OCMODE_PWM1;
  oc.Pulse = 1500;     /* 默认 90° 中间位 */
  oc.OCPolarity = TIM_OCPOLARITY_HIGH;
  oc.OCFastMode = TIM_OCFAST_DISABLE;
  HAL_TIM_PWM_ConfigChannel(&htim3, &oc, SERVO_PAN_CH);
  HAL_TIM_PWM_ConfigChannel(&htim3, &oc, SERVO_TILT_CH);

  HAL_TIM_PWM_Start(&htim3, SERVO_PAN_CH);
  HAL_TIM_PWM_Start(&htim3, SERVO_TILT_CH);
}

static uint16_t angle_to_pulse(uint8_t deg)
{
  if (deg > 180) deg = 180;
  return 500 + (uint32_t)deg * 2000 / 180;   /* 0.5ms~2.5ms */
}

void Servo_SetAngle(uint8_t pan, uint8_t tilt)
{
  __HAL_TIM_SET_COMPARE(&htim3, SERVO_PAN_CH, angle_to_pulse(pan));
  __HAL_TIM_SET_COMPARE(&htim3, SERVO_TILT_CH, angle_to_pulse(tilt));
}

uint16_t Servo_GetPWM(uint8_t ch)
{
  if (ch == 0) return __HAL_TIM_GET_COMPARE(&htim3, SERVO_PAN_CH);
  return __HAL_TIM_GET_COMPARE(&htim3, SERVO_TILT_CH);
}

void Servo_GetState(uint8_t *pan, uint8_t *tilt, uint16_t *pwm_pan, uint16_t *pwm_tilt)
{
  uint16_t pp = __HAL_TIM_GET_COMPARE(&htim3, SERVO_PAN_CH);
  uint16_t tp = __HAL_TIM_GET_COMPARE(&htim3, SERVO_TILT_CH);
  if (pwm_pan)  *pwm_pan  = pp;
  if (pwm_tilt) *pwm_tilt = tp;
  if (pan)  *pan  = (pp - 500) * 180 / 2000;
  if (tilt) *tilt = (tp - 500) * 180 / 2000;
}