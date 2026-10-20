/**
 ******************************************************************************
 * @file           : temp_control.c
 * @brief          : 温度控制系统主控制模块实现
 ******************************************************************************
 */

#include "temp_control.h"
#include "robot_config.h"
#include "pid_controller.h"
#include "ds18b20.h"
#include "bsp_pwm.h"
#include <math.h>

/* PID 控制器实例 */
static PID_Controller_t pid_controller;

/* 系统状态 */
static TempControlState_t system_state = TEMP_CTRL_IDLE;
static float target_temperature = TEMP_DEFAULT_TARGET;
static float current_temperature = 0.0f;
static bool control_enabled = false;

/* 温度稳定判断阈值 */
#define TEMP_STABLE_THRESHOLD   0.5f

/**
 * @brief  初始化温度控制系统
 */
void TempControl_Init(void)
{
    // 初始化 PID 控制器
    PID_Init(&pid_controller, PID_KP, PID_KI, PID_KD);
    PID_SetOutputLimits(&pid_controller, -100.0f, 100.0f);
    PID_SetTarget(&pid_controller, target_temperature);

    // 初始化 DS18B20
    DS18B20_Init();

    // 初始化 PWM
    BSP_PWM_Init();

    // 初始化时关闭所有输出
    BSP_PWM_SetDutyCycle(PWM_CH_FAN1, 0);
    BSP_PWM_SetDutyCycle(PWM_CH_FAN2, 0);
    BSP_PWM_SetDutyCycle(PWM_CH_HEATER, 0);
}

/**
 * @brief  设置目标温度
 */
void TempControl_SetTarget(float target_temp)
{
    // 限制温度范围
    if (target_temp < TEMP_MIN) target_temp = TEMP_MIN;
    if (target_temp > TEMP_MAX) target_temp = TEMP_MAX;

    target_temperature = target_temp;
    PID_SetTarget(&pid_controller, target_temperature);
    PID_Reset(&pid_controller);
}

/**
 * @brief  获取目标温度
 */
float TempControl_GetTarget(void)
{
    return target_temperature;
}

/**
 * @brief  获取当前温度
 */
float TempControl_GetCurrent(void)
{
    return current_temperature;
}

/**
 * @brief  更新温度控制（需要周期性调用）
 */
void TempControl_Update(void)
{
    // 读取当前温度
    if (DS18B20_IsPresent()) {
        current_temperature = DS18B20_ReadTemperature();
    } else {
        // 传感器错误，停止控制
        control_enabled = false;
        system_state = TEMP_CTRL_IDLE;
        BSP_PWM_SetDutyCycle(PWM_CH_FAN1, 0);
        BSP_PWM_SetDutyCycle(PWM_CH_FAN2, 0);
        BSP_PWM_SetDutyCycle(PWM_CH_HEATER, 0);
        return;
    }

    if (!control_enabled) {
        system_state = TEMP_CTRL_IDLE;
        return;
    }

    // 计算 PID 输出
    float pid_output = PID_Compute(&pid_controller, current_temperature);

    // 根据 PID 输出控制加热片和风扇
    if (pid_output > 0) {
        // 需要加热
        system_state = TEMP_CTRL_HEATING;

        uint8_t heater_duty = (uint8_t)pid_output;
        BSP_PWM_SetDutyCycle(PWM_CH_HEATER, heater_duty);
        BSP_PWM_SetDutyCycle(PWM_CH_FAN1, 0);
        BSP_PWM_SetDutyCycle(PWM_CH_FAN2, 0);

    } else if (pid_output < 0) {
        // 需要冷却
        system_state = TEMP_CTRL_COOLING;

        uint8_t fan_duty = (uint8_t)(-pid_output);
        BSP_PWM_SetDutyCycle(PWM_CH_HEATER, 0);
        BSP_PWM_SetDutyCycle(PWM_CH_FAN1, fan_duty);
        BSP_PWM_SetDutyCycle(PWM_CH_FAN2, fan_duty);

    } else {
        // 温度稳定
        system_state = TEMP_CTRL_STABLE;

        BSP_PWM_SetDutyCycle(PWM_CH_HEATER, 0);
        BSP_PWM_SetDutyCycle(PWM_CH_FAN1, 0);
        BSP_PWM_SetDutyCycle(PWM_CH_FAN2, 0);
    }

    // 温度稳定判断
    if (fabsf(current_temperature - target_temperature) < TEMP_STABLE_THRESHOLD) {
        if (system_state != TEMP_CTRL_STABLE) {
            system_state = TEMP_CTRL_STABLE;
        }
    }
}

/**
 * @brief  获取系统状态
 */
TempControlState_t TempControl_GetState(void)
{
    return system_state;
}

/**
 * @brief  使能/禁用温度控制
 */
void TempControl_Enable(bool enable)
{
    control_enabled = enable;

    if (!enable) {
        // 禁用时关闭所有输出
        BSP_PWM_SetDutyCycle(PWM_CH_FAN1, 0);
        BSP_PWM_SetDutyCycle(PWM_CH_FAN2, 0);
        BSP_PWM_SetDutyCycle(PWM_CH_HEATER, 0);
        system_state = TEMP_CTRL_IDLE;
        PID_Reset(&pid_controller);
    }
}
