#ifndef __MPU6050_H
#define __MPU6050_H

#include "main.h"

typedef struct {
  float pitch;   /* 俯仰角 ±90° */
  float roll;    /* 横滚角 ±180° */
  float yaw;     /* 偏航角（积分漂移，仅供参考） */
} MPU_Attitude;

uint8_t MPU_Init(void);
uint8_t MPU_ReadRaw(int16_t *accel, int16_t *gyro);
void    MPU_UpdateAttitude(MPU_Attitude *att, float dt);
void    MPU_GetAttitude(MPU_Attitude *att);
void    MPU_PushAttitude(const MPU_Attitude *att);   /* 把解算结果存进模块内全局 */
void    MPU_Calibrate(uint16_t samples);             /* 上电静止采陀螺仪零偏 */

#endif