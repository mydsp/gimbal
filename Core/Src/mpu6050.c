#include "mpu6050.h"
#include <math.h>

static I2C_HandleTypeDef hi2c1;
static MPU_Attitude attitude = {0};
static float gyro_offset[3] = {0};

/* I2C 读写 */
static uint8_t i2c_read(uint8_t reg, uint8_t *buf, uint16_t len)
{
  return HAL_I2C_Mem_Read(&hi2c1, MPU_ADDR << 1, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100);
}

static uint8_t i2c_write(uint8_t reg, uint8_t val)
{
  return HAL_I2C_Mem_Write(&hi2c1, MPU_ADDR << 1, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, 100);
}

uint8_t MPU_Init(void)
{
  __HAL_RCC_I2C1_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitTypeDef g = {0};
  g.Pin = MPU_SCL_PIN | MPU_SDA_PIN;
  g.Mode = GPIO_MODE_AF_OD;
  g.Pull = GPIO_PULLUP;
  g.Speed = GPIO_SPEED_FREQ_HIGH;
  g.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(MPU_PORT, &g);

  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;     /* 400kHz */
hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK) return 1;

  /* 复位 MPU6050 */
  i2c_write(0x6B, 0x80);
  HAL_Delay(50);
  i2c_write(0x6B, 0x00);     /* 唤醒 */
  HAL_Delay(10);
  i2c_write(0x1B, 0x00);     /* ±250°/s 陀螺仪 */
  i2c_write(0x1C, 0x00);     /* ±2g 加速度计 */
  i2c_write(0x1A, 0x03);     /* 低通滤波 44Hz */
  i2c_write(0x19, 0x07);     /* 采样率分频 1kHz/(1+7)=125Hz */

  /* 读 WHO_AM_I 确认连接 */
  uint8_t who;
  if (i2c_read(0x75, &who, 1) != HAL_OK || who != 0x68) return 1;

  HAL_Delay(100);
  return 0;
}

uint8_t MPU_ReadRaw(int16_t *accel, int16_t *gyro)
{
  uint8_t buf[14];
  if (i2c_read(0x3B, buf, 14) != HAL_OK) return 1;

  if (accel) {
    accel[0] = (int16_t)(buf[0] << 8 | buf[1]);   /* AX */
    accel[1] = (int16_t)(buf[2] << 8 | buf[3]);   /* AY */
    accel[2] = (int16_t)(buf[4] << 8 | buf[5]);   /* AZ */
  }
  if (gyro) {
    gyro[0] = (int16_t)(buf[8] << 8 | buf[9]);    /* GX */
    gyro[1] = (int16_t)(buf[10] << 8 | buf[11]);   /* GY */
    gyro[2] = (int16_t)(buf[12] << 8 | buf[13]);   /* GZ */
  }
  return 0;
}

/* 互补滤波：Pitch/Roll 跟随 */
void MPU_UpdateAttitude(MPU_Attitude *att, float dt)
{
  int16_t accel[3], gyro[3];
  if (MPU_ReadRaw(accel, gyro) != 0) return;

  float ax = accel[0] / 16384.0f;   /* ±2g 灵敏度 */
  float ay = accel[1] / 16384.0f;
  float az = accel[2] / 16384.0f;

  float gx = (gyro[0] - gyro_offset[0]) / 131.0f;   /* ±250°/s 灵敏度 */
  float gy = (gyro[1] - gyro_offset[1]) / 131.0f;
  float gz = (gyro[2] - gyro_offset[2]) / 131.0f;

  /* 加速度计算 Pitch/Roll */
  float acc_pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * 180.0f / 3.14159f;
  float acc_roll  = atan2f(ay, az) * 180.0f / 3.14159f;

  /* 互补滤波 */
  float alpha = 0.98f;
  att->pitch = alpha * (att->pitch + gy * dt) + (1 - alpha) * acc_pitch;
  att->roll  = alpha * (att->roll  + gx * dt) + (1 - alpha) * acc_roll;
  att->yaw  += gz * dt;   /* 纯积分，漂移不管 */
}

void MPU_GetAttitude(MPU_Attitude *att)
{
  if (att) *att = attitude;
}

void MPU_PushAttitude(const MPU_Attitude *att)
{
  if (att) attitude = *att;   /* 让 sensor 任务把解算结果发布给其他任务 */
}

void MPU_Calibrate(uint16_t samples)
{
  int32_t sum[3] = {0};
  int16_t accel[3], gyro[3];
  for (uint16_t i = 0; i < samples; i++) {
    if (MPU_ReadRaw(accel, gyro) == 0) {
      sum[0] += gyro[0]; sum[1] += gyro[1]; sum[2] += gyro[2];
    }
    HAL_Delay(5);
  }
  gyro_offset[0] = (float)sum[0] / samples;
  gyro_offset[1] = (float)sum[1] / samples;
  gyro_offset[2] = (float)sum[2] / samples;
}