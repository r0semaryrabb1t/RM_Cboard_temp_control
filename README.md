# STM32F103C8T6 温度控制系统

基于 STM32F103C8T6 的 PID 温度控制系统，支持蓝牙远程设置目标温度，实时显示当前温度和系统状态。

## 硬件配置

### 主控
- **MCU**: STM32F103C8T6 (72MHz, 64KB Flash, 20KB RAM)
- **外部晶振**: 8MHz HSE

### 外设连接

| 外设 | 引脚 | 功能 | 备注 |
|------|------|------|------|
| 风扇1 | PA8 | TIM1_CH1 PWM | 3.3V 供电 |
| 风扇2 | PA9 | TIM1_CH2 PWM | 3.3V 供电 |
| 加热片 | PB4 | TIM3_CH1 PWM | PWM 控制 |
| DS18B20 | PA12 | GPIO (单总线) | 温度传感器 |
| HC-06 蓝牙 | PA2/PA3 | USART2 (TX/RX) | 9600 波特率 |
| OLED 显示屏 | PB6/PB7 | I2C1 (SCL/SDA) | SSD1306, 128x64 |

## 功能特性

- ✅ PID 温度控制算法
- ✅ 蓝牙远程设置目标温度
- ✅ OLED 实时显示温度和状态
- ✅ 自动加热/冷却控制
- ✅ 温度范围保护 (0-100°C)
- ✅ 传感器故障检测

## 项目结构

```
stm32_temp_control/
├── stm32_temp_control.ioc          # CubeMX 配置
├── CMakeLists.txt                  # 构建配置
├── Core/
│   ├── Src/                        # HAL 源文件（CubeMX 生成）
│   │   ├── main.c                  # 主程序
│   │   ├── i2c.c/tim.c/usart.c     # 外设初始化
│   │   └── gpio.c
│   ├── Inc/                        # HAL 头文件
│   └── Application/                # 应用层
│       ├── bsp/                    # 板级支持
│       │   ├── bsp_pwm.c/.h
│       │   └── bsp_bluetooth.c/.h
│       ├── driver/                 # 硬件驱动
│       │   ├── ds18b20.c/.h
│       │   └── ssd1306.c/.h
│       └── control/                # 控制逻辑
│           ├── robot_config.h
│           ├── pid_controller.c/.h
│           ├── temp_control.c/.h
│           └── system_task.c/.h
└── Drivers/                        # STM32 HAL 库
```

## 快速开始

### 编译
```bash
mkdir build 
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```

### 烧录（probe-rs）
```bash
cd ~/RM_Cboard_temp_control
probe-rs download --chip STM32F103C8 --binary-format hex build/stm32_temp_control.hex
```

### 调试
```bash
# GDB 调试
probe-rs gdb --chip STM32F103C8

# 另一个终端
arm-none-eabi-gdb build/stm32_temp_control.elf
(gdb) target remote :1337
(gdb) load
(gdb) continue
```

## 使用说明

### 蓝牙通信协议

通过蓝牙串口发送以下命令（9600 波特率，每条命令以 `\r\n` 结尾）：

#### 设置目标温度
```
SET:25.5
```
响应：`OK: Target set to 25.5 C`

#### 查询温度
```
GET
```
响应：`Target:25.5 Current:24.8`

### OLED 显示内容

```
Temp Control
Target: 25.5C
Current:24.8C
State: HEATING
```

状态说明：
- `IDLE`: 空闲（控制未启用或传感器错误）
- `HEATING`: 加热中
- `COOLING`: 冷却中
- `STABLE`: 温度稳定

## 配置参数

所有配置参数在 `Core/Application/control/robot_config.h` 中定义：

### PID 参数
```c
#define PID_KP                  2.0f    // 比例系数
#define PID_KI                  0.5f    // 积分系数
#define PID_KD                  1.0f    // 微分系数
```

### 温度范围
```c
#define TEMP_MIN                0.0f    // 最低温度
#define TEMP_MAX                100.0f  // 最高温度
#define TEMP_DEFAULT_TARGET     25.0f   // 默认目标温度
```

### 更新周期
```c
#define TEMP_READ_PERIOD_MS     500     // 温度读取周期
#define DISPLAY_UPDATE_PERIOD_MS 200    // 显示更新周期
```

## 开发说明

### 修改 CubeMX 配置
```bash
# 打开 .ioc 文件修改引脚/外设配置后
# 在 CubeMX 中点击 "Generate Code"
# 应用代码不会被覆盖（已集成到 USER CODE 区域）
```

### PID 调参建议
1. 仅 P 控制: 设 `KI=0, KD=0`，增大 `KP` 至振荡
2. 加入 I: 增加 `KI` 消除稳态误差
3. 加入 D: 增加 `KD` 减少超调

配置文件: `Core/Application/control/robot_config.h`

## 注意事项

1. **DS18B20 转换时间**: 12 位精度下需要 750ms
2. **PWM 频率**: 默认 1kHz
3. **I2C 速度**: 400kHz Fast Mode
4. **蓝牙初始密码**: 1234 或 0000
5. **调试器权限**: 可能需要添加 udev 规则（见下方）

### udev 规则（Linux）
```bash
sudo tee /etc/udev/rules.d/99-stlink.rules <<EOF
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3748", MODE="0666"
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374b", MODE="0666"
EOF
sudo udevadm control --reload-rules
```

## 许可证

MIT License - 仅供学习参考

---

Created: 2026-10-20 | STM32F103C8T6 Temperature Control System
