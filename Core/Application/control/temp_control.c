#include "temp_control.h"
#include "robot_config.h"
#include "pid_controller.h"
#include "ds18b20.h"
#include "bsp_pwm.h"
#include <math.h>

static PID_Controller_t pid_controller;
static TempControlState_t system_state;
static float target_temperature;
static float current_temperature;
static bool control_enabled;
static bool temperature_valid;
static bool overheat_latched;
static uint32_t last_sample_tick;
static unsigned valid_samples;

static void outputs_off(void)
{
    BSP_PWM_SetDutyCycle(PWM_CH_HEATER, 0U);
    BSP_PWM_SetDutyCycle(PWM_CH_FAN1, 0U);
    BSP_PWM_SetDutyCycle(PWM_CH_FAN2, 0U);
}

static void sensor_fault(void)
{
    temperature_valid = false;
    valid_samples = 0U;
    system_state = TEMP_CTRL_FAULT;
    PID_Reset(&pid_controller);
    outputs_off();
}

void TempControl_Init(void)
{
    target_temperature = TEMP_DEFAULT_TARGET;
    current_temperature = 0.0f;
    control_enabled = false;
    temperature_valid = false;
    overheat_latched = false;
    valid_samples = 0U;
    last_sample_tick = HAL_GetTick();
    system_state = TEMP_CTRL_IDLE;
    PID_Init(&pid_controller, PID_KP, PID_KI, PID_KD);
    PID_SetTarget(&pid_controller, target_temperature);
    BSP_PWM_Init(); /* Clears all compares before enabling PWM. */
    DS18B20_Init();
}

void TempControl_SetTarget(float target_temp)
{
    if (!isfinite(target_temp)) return;
    if (target_temp < TEMP_MIN) target_temp = TEMP_MIN;
    if (target_temp > TEMP_MAX) target_temp = TEMP_MAX;
    target_temperature = target_temp;
    PID_SetTarget(&pid_controller, target_temperature);
    PID_Reset(&pid_controller);
    outputs_off(); /* Never keep the old command while waiting for a sample. */
}

float TempControl_GetTarget(void) { return target_temperature; }
float TempControl_GetCurrent(void) { return current_temperature; }
bool TempControl_IsTemperatureValid(void) { return temperature_valid; }
TempControlState_t TempControl_GetState(void) { return system_state; }

void TempControl_Update(void)
{
    uint32_t now = HAL_GetTick();
    if (temperature_valid &&
        (uint32_t)(now - last_sample_tick) >= TEMP_STALE_TIMEOUT_MS) {
        sensor_fault();
    }
    float sample;
    DS18B20_Status_t status = DS18B20_Poll(&sample);
    if (status == DS18B20_WAITING) return;
    if (status != DS18B20_READY || !isfinite(sample)) {
        sensor_fault();
        return;
    }

    now = HAL_GetTick();
    float dt = valid_samples == 0U ? (float)DS18B20_CONVERT_TIME_MS / 1000.0f :
        (float)(uint32_t)(now - last_sample_tick) / 1000.0f;
    current_temperature = sample;
    temperature_valid = true;
    last_sample_tick = now;
    if (valid_samples < TEMP_VALID_SAMPLES) ++valid_samples;
    if (sample >= TEMP_OVERHEAT_C) overheat_latched = true;
    if (overheat_latched) {
        system_state = TEMP_CTRL_FAULT;
        PID_Reset(&pid_controller);
        outputs_off();
        return;
    }
    if (!control_enabled || valid_samples < TEMP_VALID_SAMPLES) {
        system_state = TEMP_CTRL_IDLE;
        outputs_off();
        return;
    }

    float output = PID_Compute(&pid_controller, sample, dt);
    float error = target_temperature - sample;
    if (error > TEMP_CONTROL_BAND_C) {
        /* Heat hard while well below setpoint; keep gentle air circulation. */
        uint8_t heat = error > 2.0f ? 100U :
                       (uint8_t)(output > 0.0f ? output + 0.5f : 0U);
        BSP_PWM_SetDutyCycle(PWM_CH_FAN1, FAN_CIRCULATION_DUTY);
        BSP_PWM_SetDutyCycle(PWM_CH_FAN2, FAN_CIRCULATION_DUTY);
        BSP_PWM_SetDutyCycle(PWM_CH_HEATER, heat);
        system_state = heat ? TEMP_CTRL_HEATING : TEMP_CTRL_STABLE;
    } else if (error < -TEMP_CONTROL_BAND_C) {
        /* Overshoot: heater off first, then command rapid forced cooling. */
        BSP_PWM_SetDutyCycle(PWM_CH_HEATER, 0U);
        BSP_PWM_SetDutyCycle(PWM_CH_FAN1, FAN_COOLING_DUTY);
        BSP_PWM_SetDutyCycle(PWM_CH_FAN2, FAN_COOLING_DUTY);
        system_state = TEMP_CTRL_COOLING;
    } else {
        /* Narrow hold band: no heater, retain only gentle circulation. */
        BSP_PWM_SetDutyCycle(PWM_CH_HEATER, 0U);
        BSP_PWM_SetDutyCycle(PWM_CH_FAN1, FAN_CIRCULATION_DUTY);
        BSP_PWM_SetDutyCycle(PWM_CH_FAN2, FAN_CIRCULATION_DUTY);
        system_state = TEMP_CTRL_STABLE;
    }
}

void TempControl_Enable(bool enable)
{
    control_enabled = enable && TEMP_CONTROL_ENABLED;
    if (!enable) overheat_latched = false;
    valid_samples = 0U;
    PID_Reset(&pid_controller);
    outputs_off();
    system_state = overheat_latched ? TEMP_CTRL_FAULT : TEMP_CTRL_IDLE;
}
