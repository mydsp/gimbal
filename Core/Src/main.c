#include "main.h"
#include "pwm.h"
#include "mpu6050.h"
#include "adc.h"
#include "gimbal.h"
#include <stdbool.h>

/* ====== FreeRTOS 头文件 ====== */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* ====== 全局 ====== */
UART_HandleTypeDef huart1;
static QueueHandle_t cmd_queue = NULL;

/* 时钟配置 */
static void SystemClock_Config(void);
static void UART1_Init(void);
static void delay_loop(volatile uint32_t n);

/* FreeRTOS 任务 */
static void vTaskComm(void *pv);
static void vTaskSensor(void *pv);
static void vTaskControl(void *pv);

int main(void)
{
  HAL_Init();
  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
  SystemClock_Config();
  UART1_Init();
  PWM_Init();
  ADC_Init();
  Gimbal_Init();

  /* 模式切换按键 PB0：内部上拉，按下接地 */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIO_InitTypeDef key_io = {0};
  key_io.Pin = GPIO_PIN_0;
  key_io.Mode = GPIO_MODE_INPUT;
  key_io.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &key_io);

  if (MPU_Init() != 0) {
    /* MPU6050 初始化失败，串口报警 */
    char msg[] = "MPU6050 FAIL\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t *)msg, sizeof(msg) - 1, 100);
  }

  cmd_queue = xQueueCreate(4, sizeof(uint8_t[2]));

  xTaskCreate(vTaskComm,    "comm",    STACK_SIZE, NULL, TASK_PRIO_COMM,   NULL);
  xTaskCreate(vTaskSensor,  "sensor",  STACK_SIZE, NULL, TASK_PRIO_SENSOR, NULL);
  xTaskCreate(vTaskControl, "control", STACK_SIZE, NULL, TASK_PRIO_CTRL,   NULL);

  vTaskStartScheduler();
  while (1) { }
}

/* 通信任务：VOFA+ 双向 JustFloat */
static void vTaskComm(void *pv)
{
  TickType_t last = xTaskGetTickCount();
  GimbalState s;

  while (1) {
    Gimbal_GetState(&s);

    /* 回传：角度 + PWM 占空比（JustFloat 4通道） */
    float tx[4] = {
      (float)s.pan_deg, (float)s.tilt_deg,
      (float)s.pan_pwm, (float)s.tilt_pwm
    };
    uint8_t tail[4] = {0x00, 0x00, 0x80, 0x7F};
    HAL_UART_Transmit(&huart1, (uint8_t *)tx, sizeof(tx), 20);
    HAL_UART_Transmit(&huart1, tail, 4, 20);

    /* 读 VOFA+ 发来的目标角度（非阻塞，有就读） */
    uint8_t buf[8];
    if (HAL_UART_Receive(&huart1, buf, 8, 0) == HAL_OK) {
      /* 协议：前4字节 pan float，后4字节 tilt float */
      float *p = (float *)buf;
      uint8_t pan = (uint8_t)(*p);
      uint8_t tilt = (uint8_t)(*(p + 1));
      uint8_t cmd[2] = {pan, tilt};
      xQueueSend(cmd_queue, cmd, 0);
    }

    vTaskDelayUntil(&last, pdMS_TO_TICKS(20));
  }
}

/* 传感器任务：MPU6050 姿态更新（static 滤波状态跨周期保留，结果发布给全局） */
static void vTaskSensor(void *pv)
{
  TickType_t last = xTaskGetTickCount();
  static MPU_Attitude att = {0};

  MPU_Calibrate(200);   /* 上电静置 1 秒采零偏，期间别动板子 */

  while (1) {
    MPU_UpdateAttitude(&att, 0.01f);
    MPU_PushAttitude(&att);
    vTaskDelayUntil(&last, pdMS_TO_TICKS(10));
  }
}

/* 控制任务：模式切换 + 舵机控制 */
static void vTaskControl(void *pv)
{
  TickType_t last = xTaskGetTickCount();
  uint8_t mode = 0;   /* 0=电位器, 1=姿态, 2=VOFA */

  while (1) {
    /* 物理按键切模式：电位器 -> 姿态 -> VOFA 循环（消抖靠 10ms 任务周期天然完成） */
    static bool key_last = false;
    bool key_now = (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET);
    if (key_now && !key_last) {
      mode = (mode + 1) % 3;
      Gimbal_SetMode((GimbalMode)mode);
    }
    key_last = key_now;

    /* 读 VOFA+ 命令切换模式 */
    uint8_t cmd[2];
    if (xQueueReceive(cmd_queue, cmd, 0) == pdTRUE) {
      /* 约定：pan=255 表示模式切换，tilt 指定模式 */
      if (cmd[0] == 255) {
        mode = cmd[1] % 3;
        Gimbal_SetMode((GimbalMode)mode);
      } else {
        /* VOFA 模式直接设角度 */
        if (mode == 2) Gimbal_VOFAControl(cmd[0], cmd[1]);
      }
    }

    /* 按钮检测（PC13 或 PB0），长按切模式 */
    /* 暂缺物理按钮，先用 VOFA+ 切 */

    switch (mode) {
      case 0: Gimbal_PotControl(); break;
      case 1: Gimbal_AttitudeControl(); break;
      /* case 2: 由 VOFA 命令驱动 */
      default: break;
    }

    vTaskDelayUntil(&last, pdMS_TO_TICKS(10));
  }
}

/* ====== 外设初始化 ====== */
static void UART1_Init(void)
{
  __HAL_RCC_USART1_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitTypeDef g = {0};
  g.Pin = GPIO_PIN_9 | GPIO_PIN_10;
  g.Mode = GPIO_MODE_AF_PP;
  g.Pull = GPIO_PULLUP;
  g.Speed = GPIO_SPEED_FREQ_HIGH;
  g.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOA, &g);

  huart1.Instance = USART1;
  huart1.Init.BaudRate = UART_BAUD;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  HAL_UART_Init(&huart1);
}

static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

void Error_Handler(void) { while (1) { } }