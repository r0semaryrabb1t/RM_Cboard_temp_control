#include "pid_controller.h"
#include "robot_config.h"
#include <math.h>

static float clamp(float value, float low, float high)
{
    return value < low ? low : (value > high ? high : value);
}

void PID_Init(PID_Controller_t *pid, float kp, float ki, float kd)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->setpoint = 0.0f;
    pid->output_min = PID_OUTPUT_MIN;
    pid->output_max = PID_OUTPUT_MAX;
    pid->integral_limit = PID_INTEGRAL_LIMIT;
    PID_Reset(pid);
}

void PID_SetTarget(PID_Controller_t *pid, float setpoint)
{
    pid->setpoint = setpoint;
}

/* dt is elapsed seconds between fresh samples, not main-loop iterations. */
float PID_Compute(PID_Controller_t *pid, float measured_value, float dt)
{
    if (!isfinite(measured_value) || !isfinite(pid->setpoint) ||
        !isfinite(dt) || dt <= 0.0f) {
        PID_Reset(pid);
        return 0.0f;
    }
    float error = pid->setpoint - measured_value;
    float candidate = clamp(pid->integral + error * dt,
                            -pid->integral_limit, pid->integral_limit);
    /* Derivative on measurement avoids setpoint kick; no first-sample kick. */
    float derivative = pid->has_previous ?
        -pid->kd * (measured_value - pid->prev_measurement) / dt : 0.0f;
    float output = pid->kp * error + pid->ki * candidate + derivative;
    /* Stop integrating into saturation, but allow the integral to unwind. */
    if (!((output > pid->output_max && error > 0.0f) ||
          (output < pid->output_min && error < 0.0f))) {
        pid->integral = candidate;
    }
    output = pid->kp * error + pid->ki * pid->integral + derivative;
    pid->prev_measurement = measured_value;
    pid->has_previous = true;
    return clamp(output, pid->output_min, pid->output_max);
}

void PID_Reset(PID_Controller_t *pid)
{
    pid->integral = 0.0f;
    pid->prev_measurement = 0.0f;
    pid->has_previous = false;
}

void PID_SetOutputLimits(PID_Controller_t *pid, float min, float max)
{
    pid->output_min = min;
    pid->output_max = max;
}
