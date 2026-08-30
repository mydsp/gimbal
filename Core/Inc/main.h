#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f4xx_hal.h"

/* ====== 引脚映射 ====== */
#define LED_PIN              GPIO_PIN_5
#define LED_PORT             GPIOA

/* 舵机 PWM：TIM3, 50Hz */
#define SERVO_PAN_PIN        GPIO_PIN_6   /* PA6 = TIM3_CH1 */
#define SERVO_TILT_PIN       GPIO_PIN_7   /* PA7 = TIM3_CH2 */
#define SERVO_PORT           GPIOA
#define SERVO_TIM            TIM3
#define SERVO_PAN_CH         TIM_CHANNEL_1
#define SERVO_TILT_CH        TIM_CHANNEL_2

/* 电位器 ADC：ADC1 */
#define POT_PAN_PIN          GPIO_PIN_0   /* PA0 = ADC1_IN0 */
#define POT_TILT_PIN         GPIO_PIN_1   /* PA1 = ADC1_IN1 */
#define POT_PORT             GPIOA

/* MPU6050：I2C1 */
#define MPU_I2C              I2C1
#define MPU_SCL_PIN          GPIO_PIN_6   /* PB6 */
#define MPU_SDA_PIN          GPIO_PIN_7   /* PB7 */
#define MPU_PORT             GPIOB
#define MPU_ADDR             0x68

/* USART1：BT24 / DAPLink VCP */
#define UART_BAUD            115200

/* FreeRTOS 任务优先级 */
#define TASK_PRIO_CTRL       (tskIDLE_PRIORITY + 2)
#define TASK_PRIO_SENSOR     (tskIDLE_PRIORITY + 3)
#define TASK_PRIO_COMM       (tskIDLE_PRIORITY + 1)
#define STACK_SIZE           256

#endif