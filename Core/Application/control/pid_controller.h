/**
 ******************************************************************************
 * @file           : pid_controller.h
 * @brief          : PID 温度控制器
 ******************************************************************************
 */

#ifndef __PID_CONTROLLER_H
#define __PID_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* PID 控制器结构体 */
typedef struct {
    float kp;                // 比例系数
    float ki;                // 积分系数
    float kd;                // 微分系数
    float setpoint;          // 目标值
    float integral;          // 积分累积
    float prev_measurement;  // 上一次有效采样
    bool has_previous;       // 微分项初始化标记
    float output_min;        // 输出最小值
    float output_max;        // 输出最大值
    float integral_limit;    // 积分限幅
} PID_Controller_t;

/* 函数声明 */
void PID_Init(PID_Controller_t *pid, float kp, float ki, float kd);
void PID_SetTarget(PID_Controller_t *pid, float setpoint);
float PID_Compute(PID_Controller_t *pid, float measured_value, float dt_seconds);
void PID_Reset(PID_Controller_t *pid);
void PID_SetOutputLimits(PID_Controller_t *pid, float min, float max);

#ifdef __cplusplus
}
#endif

#endif /* __PID_CONTROLLER_H */
