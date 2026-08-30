# gimbal —— 软5：二自由度云台·单板全功能（STM32F401RET6 + FreeRTOS）

> 一句话：三任务（通信/姿态/控制）三模式（电位器 / MPU6050 姿态 / VOFA+）单板云台；双板无线拆分方案见 Obsidian《考核题跟做/05-软5_二自由度云台》。
> 对应《G308 电控组 2026 夏季考核题》软件第五题。

## 当前状态

- 编译通过：`Flash 28.7KB / 5.48%`（2026-08-30）
- 2026-08-30 已修复三个问题（commit e7a7d38）：
  1. **姿态持久化**：原 `vTaskSensor` 更新局部变量即丢弃，全局姿态永远是 0 → 新增 `MPU_PushAttitude()` 发布
  2. **陀螺仪零偏**：`gyro_offset` 从未赋值 → 新增 `MPU_Calibrate(200)`，上电静置 1 秒自动采零偏
  3. **物理按键切模式**：PB0（内部上拉，按下接地）循环切换 电位器→姿态→VOFA

## 架构

| 任务 | 周期 | 职责 |
| --- | --- | --- |
| vTaskComm | 20ms | 回传四通道 JustFloat（pan/tilt 角度+脉宽）；收 VOFA+ 目标与模式命令 |
| vTaskSensor | 10ms | MPU6050 原始数据 → 互补滤波 → 发布姿态 |
| vTaskControl | 10ms | 按键/命令切模式 → 按模式驱动舵机 |

## 接线（引脚级）

| F401 | 去向 | 备注 |
| --- | --- | --- |
| PA6 / PA7 (TIM3) | 舵机 pan / tilt 信号 | 50Hz，500~2500µs |
| PA0 / PA1 (ADC1) | 电位器中间脚 ×2 | 两端接 3.3V/GND，**不接 5V** |
| PB6 / PB7 (I2C1) | MPU6050 SCL/SDA | 400kHz，AD0 悬空=地址 0x68 |
| PA9 / PA10 | 串口 / BT24 | 115200 |
| PB0 | 按键 → GND | 切模式 |

## 构建与烧录

```powershell
cmake --build build\Debug
pyocd flash -t stm32f401retx build\Debug\gimbal.bin
```

（首次配置：`cmake -S . -B build\Debug -G Ninja -DCMAKE_MAKE_PROGRAM=E:\STM32CubeCLT_1.21.0\Ninja\bin\ninja.exe -DCMAKE_TOOLCHAIN_FILE=cmake\gcc-arm-none-eabi.cmake`）

## 已知限制

- 双板无线拆分（题目硬性要求）未落码，帧协议与两侧代码已定稿在笔记中
- 姿态跟随方向符号 `90 ± att` 需按机械装配实测校正
- FreeRTOS 内核取自本机 CubeFW F4 的 Middlewares，换机编译改 `CMakeLists.txt` 的 `FW_DIR`
