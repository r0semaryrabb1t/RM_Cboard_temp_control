# STM32F103C8T6：30°C 温度控制

当前入口已改为 **DS18B20 采样 → PID → PWM**。OLED 只显示当前温度与预设温度，
默认预设 **30.0°C**。不再运行蓝牙独立测试，也不依赖已删除的蓝牙驱动。

## 接线与构成

以下是本工程的引脚定义，不代表已确认实物接线：

| 外设 | MCU 引脚 | 用途 |
| --- | --- | --- |
| DS18B20 DQ | **PA12** | 单总线，外接约 4.7kΩ 上拉到 3.3V |
| DS18B20 VDD / GND | 3.3V / GND | 三线独立供电，本驱动不支持寄生供电 |
| OLED SCL / SDA | PB6 / PB7 | I2C1，SSD1306 128×64，7 位地址 0x3C |
| 风扇 1 控制 | PA8 | TIM1_CH1，固件按低有效输出配置；需与驱动板输入极性核对 |
| 风扇 2 控制 | PA9 | TIM1_CH2，固件按低有效输出配置；需与驱动板输入极性核对 |
| 加热片控制 | PB4 | TIM3_CH1，部分重映射，高电平有效 PWM |
| ST-Link SWDIO / SWCLK | PA13 / PA14 | 保留 SWD；关闭 JTAG 以释放 PB4 |
| 可选串口 TX / RX | PA2 / PA3 | USART2，当前任务不使用蓝牙协议 |

**商家代码的 DQ 是 PB6；本工程 PB6 已被 OLED 的 SCL 占用。**
参考代码位于 `/home/rosemaryrabbit/测试程序/System/ds18b20.c` 与 `.h`。
不能将商家引脚定义直接复制进来；请将传感器 DQ 接到 PA12，或统一调整工程和接线，不能共用 PB6。
商家示例的温度变量以 0.1°C 为单位，本工程内部使用摄氏度浮点数。

MCU 配置为 STM32F103C8T6、8MHz HSE、72MHz 系统时钟，不是 RoboMaster C 板固件。
三个 PWM 通道当前均为 **1kHz**，范围 0–100%。100% 对应 CCR=ARR+1。
PWM 引脚只输出逻辑控制信号，**不可直接给加热片或风扇供电**；须使用匹配的 MOSFET/驱动模块、
额定电源并共地。当前代码假设风扇驱动输入为低有效（PWM 反相），风扇 PWM 为 1kHz；实际极性及起转占空比仍须按驱动板规格验证。
PWM 改变驱动级的平均功率，并不是 MCU 引脚输出可调模拟电压。

## 先验证温度读取（建议第一步）

构建一个强制禁止控制输出的版本，避免接线排查时意外加热：

```bash
cmake -S . -B build/sensor-only -DCMAKE_BUILD_TYPE=Debug -DTEMP_SENSOR_ONLY=ON
cmake --build build/sensor-only -j4
probe-rs download --chip STM32F103C8 --verify build/sensor-only/stm32_temp_control
probe-rs reset --chip STM32F103C8
```

此版本中加热片和两个风扇的占空比始终为 0%，即使调用 `TempControl_Enable(true)` 也不能打开控制。
首先显示 `CURR:WAIT` / `SET:30.0C`，获得有效采样后例如：

```text
CURR:25.1C
SET:30.0C
```

这里只是显示格式示例，不是实测读数。每次转换等待至少 750ms，采样通常约 0.8 秒一轮；
屏幕每 200ms 尝试刷新，显示一位小数，不再使用“变化超过 0.2°C 才刷新”的门槛。
用手握住探头观察读数变化，再放开观察回落。实际刷新速度受 I2C 操作影响。

故障时只替换当前温度这一行，不把旧值或错误值当作当前温度：

| 显示 | 含义 / 检查方向 |
| --- | --- |
| `CURR:WAIT` | 正等待首次转换 |
| `CURR:NO SENSOR` | 未检测到有效复位响应；检查 PA12、供电、共地、上拉、短路 |
| `CURR:CRC ERR` | 9 字节读取后的 CRC 不匹配；检查连接和信号质量 |
| `CURR:TIMEOUT` | 转换未完成或转换结果过期 |
| `CURR:DATA ERR` | 数据内容不合法 |

GPIO 使用开漏，DWT 周期计数器提供微秒延时；不会修改 HAL 的 SysTick。
读写时隙短暂屏蔽中断并恢复原中断状态；750ms 转换等待不阻塞主循环。
驱动检查复位响应、转换完成、完整 9 字节 CRC、配置位与 -55～125°C 数据范围，
拒绝全零/全一错误数据，并支持负温和 9～12 位分辨率。
总线使用 SKIP ROM，只支持单个传感器。故障后每秒重试。

## 启用 30°C PID 控温

确认传感器数据会随实际温度变化、探头与被控物体热接触可靠、驱动极性/功率连接正确后，再烧录：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DTEMP_SENSOR_ONLY=OFF
cmake --build build -j4
probe-rs download --chip STM32F103C8 --verify build/stm32_temp_control
probe-rs reset --chip STM32F103C8
```

- 默认目标：30.0°C，统一定义于 `Core/Application/control/robot_config.h`。
- 单一采样所有者：`TempControl_Update()`；显示和 PID 使用同一份已校验温度。
- 上电及传感器故障恢复后，需要连续两次有效采样才允许 PWM 输出。
- PID 按**有效采样实际间隔（秒）**计算，含积分限幅、输出饱和抗积分累积、测量值微分。
- 低于设定温度 0.3°C 以上时 PID 控制加热，差值大于 2°C 时加热 100%；风扇维持 20% 循环风。高于设定值 0.3°C 时先关加热再全速风冷；±0.3°C 内关加热、风扇保持 20%。
- 在目标附近保留 PID 维持功率，不因落入稳定带就强制停热。
- 任一读温错误立即将三路占空比置零；最后有效采样超过 2500ms 也停止输出。
- 当前软件超温阈值为 **45°C**，触发后锁定三路输出为零；即使温度回落也不自动恢复。
  排查原因后，可重启或显式调用 `TempControl_Enable(false)` 再 `TempControl_Enable(true)`；
  温度仍超限时下一次有效采样会再次锁定。目标设置范围暂限 0～40°C。
- OLED 故障不阻塞温度任务，显示初始化每秒重试。

**Kp=2.0、Ki=0.5、Kd=1.0 是初始参数，不是实物整定结果。**
能否稳定在 30°C 及误差范围，取决于加热功率、热惯性、探头位置、散热和环境温度，须实测调整。
普通风扇不能保证将被控对象冷却到低于环境温度；低占空比也可能不足以起转。
45°C 软件阈值不是装置安全认证；软件保护依赖主循环正常运行，无法防止程序卡死、驱动短路、
探头脱离被控物体等故障。首次加热务必有人监护，使用适当的硬件过温断电与限流措施。

## 关键代码

- `Core/Src/main.c`：HAL/外设初始化，然后循环运行系统任务。
- `Core/Application/driver/ds18b20_bus.c`：PA12 单总线电气/时序层。
- `Core/Application/driver/ds18b20.c`：非阻塞转换、校验、错误重试。
- `Core/Application/control/temp_control.c`：新采样门控、保护、PID 与执行器控制。
- `Core/Application/control/pid_controller.c`：按秒计算的 PID。
- `Core/Application/bsp/bsp_pwm.c`：PWM 占空比及启动前清零。
- `Core/Application/control/system_task.c`：两行温度显示、OLED 重试。

## 本机回归测试

```bash
sh tests/run_temperature_tests.sh
sh tests/run_oled_tests.sh
```

温度测试分别覆盖只读和控制模式：等待转换、命令顺序、断线、CRC、全零/全一、负温、分辨率、
计时回绕、超时、恢复门控、超温锁定、PID 采样周期、积分饱和、PWM 0/100% 和两行显示更新。
测试使用宿主机 ASan/UBSan，模拟总线/定时器/OLED，不等同于对物理单总线波形或闭环稳定性的验证。
旧 `run_bluetooth_tests.sh` 依赖此前已删除的蓝牙驱动，不属于当前温控回归入口。

构建输出的 `stm32_temp_control` 是无后缀 ELF，同时生成 `.hex` / `.bin`。
