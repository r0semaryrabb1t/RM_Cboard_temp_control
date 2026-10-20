/**
 ******************************************************************************
 * @file           : ds18b20.h
 * @brief          : DS18B20 温度传感器驱动
 ******************************************************************************
 */

#ifndef __DS18B20_H
#define __DS18B20_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/* DS18B20 ROM 命令 */
#define DS18B20_CMD_SKIP_ROM        0xCC
#define DS18B20_CMD_CONVERT_T       0x44
#define DS18B20_CMD_READ_SCRATCHPAD 0xBE

/* 函数声明 */
void DS18B20_Init(void);
bool DS18B20_IsPresent(void);
void DS18B20_StartConversion(void);
float DS18B20_ReadTemperature(void);
bool DS18B20_ReadTemperatureNonBlocking(float *temp);

#ifdef __cplusplus
}
#endif

#endif /* __DS18B20_H */
