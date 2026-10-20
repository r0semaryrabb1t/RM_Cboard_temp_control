/**
 ******************************************************************************
 * @file           : bsp_pwm.h
 * @brief          : PWM 控制板级支持包
 ******************************************************************************
 */

#ifndef __BSP_PWM_H
#define __BSP_PWM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>

/* PWM 通道定义 */
typedef enum {
    PWM_CH_FAN1 = 0,    // TIM1_CH1 - PA8
    PWM_CH_FAN2,        // TIM1_CH2 - PA9
    PWM_CH_HEATER,      // TIM3_CH1 - PB4
    PWM_CH_MAX
} PWM_Channel_t;

/* 函数声明 */
void BSP_PWM_Init(void);
void BSP_PWM_SetDutyCycle(PWM_Channel_t channel, uint8_t duty);
void BSP_PWM_Start(PWM_Channel_t channel);
void BSP_PWM_Stop(PWM_Channel_t channel);
void BSP_PWM_StartAll(void);
void BSP_PWM_StopAll(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_PWM_H */
