/**
 ******************************************************************************
 * @file           : bsp_pwm.c
 * @brief          : PWM 控制板级支持包实现
 ******************************************************************************
 */

#include "bsp_pwm.h"
#include "robot_config.h"
#include "main.h"

/* 外部定时器句柄 (由 CubeMX 生成，定义在 main.c) */
extern TIM_HandleTypeDef htim1;  // 风扇1、风扇2
extern TIM_HandleTypeDef htim3;  // 加热片

/**
 * @brief  初始化 PWM
 */
void BSP_PWM_Init(void)
{
    // CubeMX 已完成定时器初始化，这里启动 PWM 输出
    BSP_PWM_StartAll();
}

/**
 * @brief  设置 PWM 占空比
 * @param  channel: PWM 通道
 * @param  duty: 占空比 (0-100)
 */
void BSP_PWM_SetDutyCycle(PWM_Channel_t channel, uint8_t duty)
{
    if (duty > 100) duty = 100;

    uint32_t pulse = (uint32_t)(PWM_PERIOD * duty / 100);

    switch (channel) {
        case PWM_CH_FAN1:
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse);
            break;

        case PWM_CH_FAN2:
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, pulse);
            break;

        case PWM_CH_HEATER:
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse);
            break;

        default:
            break;
    }
}

/**
 * @brief  启动指定 PWM 通道
 */
void BSP_PWM_Start(PWM_Channel_t channel)
{
    switch (channel) {
        case PWM_CH_FAN1:
            HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
            break;

        case PWM_CH_FAN2:
            HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
            break;

        case PWM_CH_HEATER:
            HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
            break;

        default:
            break;
    }
}

/**
 * @brief  停止指定 PWM 通道
 */
void BSP_PWM_Stop(PWM_Channel_t channel)
{
    switch (channel) {
        case PWM_CH_FAN1:
            HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
            break;

        case PWM_CH_FAN2:
            HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
            break;

        case PWM_CH_HEATER:
            HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
            break;

        default:
            break;
    }
}

/**
 * @brief  启动所有 PWM 通道
 */
void BSP_PWM_StartAll(void)
{
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);  // 风扇1
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);  // 风扇2
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);  // 加热片
}

/**
 * @brief  停止所有 PWM 通道
 */
void BSP_PWM_StopAll(void)
{
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
}
