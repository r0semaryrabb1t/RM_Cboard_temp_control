/**
 ******************************************************************************
 * @file           : pid_controller.c
 * @brief          : PID 温度控制器实现
 ******************************************************************************
 */

#include "pid_controller.h"
#include "robot_config.h"

/**
 * @brief  初始化 PID 控制器
 */
void PID_Init(PID_Controller_t *pid, float kp, float ki, float kd)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->setpoint = 0.0f;
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->output_min = PID_OUTPUT_MIN;
    pid->output_max = PID_OUTPUT_MAX;
    pid->integral_limit = PID_INTEGRAL_LIMIT;
}

/**
 * @brief  设置目标值
 */
void PID_SetTarget(PID_Controller_t *pid, float setpoint)
{
    pid->setpoint = setpoint;
}

/**
 * @brief  计算 PID 输出
 * @param  measured_value: 当前测量值
 * @return PID 输出值
 */
float PID_Compute(PID_Controller_t *pid, float measured_value)
{
    float error = pid->setpoint - measured_value;

    // 比例项
    float p_term = pid->kp * error;

    // 积分项（带限幅）
    pid->integral += error;
    if (pid->integral > pid->integral_limit) {
        pid->integral = pid->integral_limit;
    } else if (pid->integral < -pid->integral_limit) {
        pid->integral = -pid->integral_limit;
    }
    float i_term = pid->ki * pid->integral;

    // 微分项
    float d_term = pid->kd * (error - pid->prev_error);
    pid->prev_error = error;

    // PID 输出
    float output = p_term + i_term + d_term;

    // 输出限幅
    if (output > pid->output_max) {
        output = pid->output_max;
    } else if (output < pid->output_min) {
        output = pid->output_min;
    }

    return output;
}

/**
 * @brief  复位 PID 控制器
 */
void PID_Reset(PID_Controller_t *pid)
{
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
}

/**
 * @brief  设置输出限幅
 */
void PID_SetOutputLimits(PID_Controller_t *pid, float min, float max)
{
    pid->output_min = min;
    pid->output_max = max;
}
