/**
 ******************************************************************************
 * @file           : temp_control.h
 * @brief          : 温度控制系统主控制模块
 ******************************************************************************
 */

#ifndef __TEMP_CONTROL_H
#define __TEMP_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* 系统状态 */
typedef enum {
    TEMP_CTRL_IDLE = 0,
    TEMP_CTRL_HEATING,
    TEMP_CTRL_COOLING,
    TEMP_CTRL_STABLE,
    TEMP_CTRL_FAULT
} TempControlState_t;

/* 函数声明 */
void TempControl_Init(void);
void TempControl_SetTarget(float target_temp);
float TempControl_GetTarget(void);
float TempControl_GetCurrent(void);
bool TempControl_IsTemperatureValid(void);
void TempControl_Update(void);
TempControlState_t TempControl_GetState(void);
void TempControl_Enable(bool enable);

#ifdef __cplusplus
}
#endif

#endif /* __TEMP_CONTROL_H */
