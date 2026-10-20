/**
 ******************************************************************************
 * @file           : bsp_bluetooth.h
 * @brief          : HC-06 蓝牙模块 UART 通信板级支持包
 ******************************************************************************
 */

#ifndef __BSP_BLUETOOTH_H
#define __BSP_BLUETOOTH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/* 回调函数类型 */
typedef void (*BT_RxCallback_t)(uint8_t *data, uint16_t len);

/* 函数声明 */
void BSP_Bluetooth_Init(void);
void BSP_Bluetooth_RegisterRxCallback(BT_RxCallback_t callback);
void BSP_Bluetooth_Transmit(uint8_t *data, uint16_t len);
void BSP_Bluetooth_TransmitString(const char *str);
void BSP_Bluetooth_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_BLUETOOTH_H */
