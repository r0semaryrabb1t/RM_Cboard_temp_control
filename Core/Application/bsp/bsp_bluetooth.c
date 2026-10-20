/**
 ******************************************************************************
 * @file           : bsp_bluetooth.c
 * @brief          : HC-06 蓝牙模块 UART 通信实现
 ******************************************************************************
 */

#include "bsp_bluetooth.h"
#include "robot_config.h"
#include "main.h"
#include <string.h>

/* 外部 UART 句柄 (定义在 main.c) */
extern UART_HandleTypeDef huart2;

/* 接收缓冲区 */
static uint8_t rx_buffer[BLUETOOTH_RX_BUFFER_SIZE];
static uint8_t rx_byte;
static uint16_t rx_index = 0;

/* 回调函数指针 */
static BT_RxCallback_t rx_callback = NULL;

/**
 * @brief  初始化蓝牙模块
 */
void BSP_Bluetooth_Init(void)
{
    // 启动 UART 中断接收
    HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
}

/**
 * @brief  注册接收回调函数
 */
void BSP_Bluetooth_RegisterRxCallback(BT_RxCallback_t callback)
{
    rx_callback = callback;
}

/**
 * @brief  发送数据
 */
void BSP_Bluetooth_Transmit(uint8_t *data, uint16_t len)
{
    HAL_UART_Transmit(&huart2, data, len, HAL_MAX_DELAY);
}

/**
 * @brief  发送字符串
 */
void BSP_Bluetooth_TransmitString(const char *str)
{
    BSP_Bluetooth_Transmit((uint8_t*)str, strlen(str));
}

/**
 * @brief  UART 中断处理（需要在 stm32f1xx_it.c 中调用）
 */
void BSP_Bluetooth_IRQHandler(void)
{
    // 接收到一个字节
    rx_buffer[rx_index++] = rx_byte;

    // 检查是否接收到命令结束符
    if (rx_byte == BT_CMD_TERMINATOR || rx_index >= BLUETOOTH_RX_BUFFER_SIZE - 1) {
        rx_buffer[rx_index] = '\0';

        // 调用回调函数处理接收到的数据
        if (rx_callback != NULL) {
            rx_callback(rx_buffer, rx_index);
        }

        // 重置接收索引
        rx_index = 0;
    }

    // 继续接收下一个字节
    HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
}

/**
 * @brief  HAL UART 接收完成回调（弱函数重写）
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        BSP_Bluetooth_IRQHandler();
    }
}
