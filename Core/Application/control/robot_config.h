/**
 ******************************************************************************
 * @file           : robot_config.h
 * @brief          : 温度控制系统全局配置参数
 * @author         : STM32 Temperature Control System
 * @date           : 2026-10-20
 ******************************************************************************
 */

#ifndef __ROBOT_CONFIG_H
#define __ROBOT_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 硬件配置 ==================== */

// 系统时钟
#define SYSTEM_CLOCK_HZ         72000000U

// PWM 频率配置 (1kHz)
#define PWM_FREQUENCY_HZ        1000U
#define PWM_PRESCALER           71U
#define PWM_PERIOD              999U

/* ==================== 温度控制参数 ==================== */

// 温度范围限制 (°C)
#define TEMP_MIN                0.0f
#define TEMP_MAX                100.0f
#define TEMP_DEFAULT_TARGET     25.0f

// 温度更新周期 (ms)
#define TEMP_READ_PERIOD_MS     500U

// PID 控制参数
#define PID_KP                  2.0f
#define PID_KI                  0.5f
#define PID_KD                  1.0f
#define PID_OUTPUT_MIN          0.0f
#define PID_OUTPUT_MAX          100.0f
#define PID_INTEGRAL_LIMIT      50.0f

/* ==================== 风扇控制参数 ==================== */

// 风扇 PWM 范围 (0-100%)
#define FAN_PWM_MIN             0U
#define FAN_PWM_MAX             100U
#define FAN_DUTY_CYCLE_MAX      100U

/* ==================== 加热片控制参数 ==================== */

// 加热片 PWM 范围 (0-100%)
#define HEATER_PWM_MIN          0U
#define HEATER_PWM_MAX          100U
#define HEATER_DUTY_CYCLE_MAX   100U

/* ==================== 蓝牙通信参数 ==================== */

// UART 配置
#define BLUETOOTH_UART_BAUDRATE 9600U
#define BLUETOOTH_RX_BUFFER_SIZE 128U

// 命令格式: "SET:XX.X\r\n" 或 "GET\r\n"
#define BT_CMD_SET_PREFIX       "SET:"
#define BT_CMD_GET              "GET"
#define BT_CMD_TERMINATOR       '\n'

/* ==================== OLED 显示配置 ==================== */

// I2C 地址 (SSD1306 默认地址)
#define OLED_I2C_ADDRESS        0x78U  // 7-bit address << 1
#define OLED_WIDTH              128U
#define OLED_HEIGHT             64U

// 显示更新周期 (ms)
#define DISPLAY_UPDATE_PERIOD_MS 200U

/* ==================== DS18B20 配置 ==================== */

// 单总线延时时序 (us)
#define DS18B20_RESET_DELAY_US  480U
#define DS18B20_PRESENCE_DELAY_US 60U
#define DS18B20_WRITE_1_DELAY_US 10U
#define DS18B20_WRITE_0_DELAY_US 60U
#define DS18B20_READ_DELAY_US   15U

// 温度转换时间 (ms)
#define DS18B20_CONVERT_TIME_MS 750U

/* ==================== 调试选项 ==================== */

// 启用调试输出 (通过串口)
#define DEBUG_ENABLE            0

#if DEBUG_ENABLE
#define DEBUG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __ROBOT_CONFIG_H */
