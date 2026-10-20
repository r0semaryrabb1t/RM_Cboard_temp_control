/**
 ******************************************************************************
 * @file           : system_task.c
 * @brief          : 系统任务调度模块实现
 ******************************************************************************
 */

#include "system_task.h"
#include "robot_config.h"
#include "temp_control.h"
#include "bsp_bluetooth.h"
#include "ssd1306.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 任务周期计数器 */
static uint32_t last_temp_update_tick = 0;
static uint32_t last_display_update_tick = 0;

/**
 * @brief  初始化系统任务
 */
void SystemTask_Init(void)
{
    // 初始化温度控制
    TempControl_Init();

    // 初始化蓝牙
    BSP_Bluetooth_Init();
    BSP_Bluetooth_RegisterRxCallback(SystemTask_BluetoothRxCallback);

    // 初始化 OLED 显示
    SSD1306_Init();
    SSD1306_Clear();
    SSD1306_WriteString(0, 0, "Temp Control", SSD1306_COLOR_WHITE);
    SSD1306_WriteString(0, 16, "System Ready", SSD1306_COLOR_WHITE);
    SSD1306_UpdateScreen();

    HAL_Delay(1000);

    // 启动温度控制
    TempControl_Enable(true);
}

/**
 * @brief  系统主循环任务
 */
void SystemTask_Run(void)
{
    uint32_t current_tick = HAL_GetTick();

    // 温度控制更新任务
    if (current_tick - last_temp_update_tick >= TEMP_READ_PERIOD_MS) {
        last_temp_update_tick = current_tick;
        TempControl_Update();
    }

    // 显示更新任务
    if (current_tick - last_display_update_tick >= DISPLAY_UPDATE_PERIOD_MS) {
        last_display_update_tick = current_tick;

        // 更新 OLED 显示
        char buffer[32];

        SSD1306_Clear();

        // 显示标题
        SSD1306_WriteString(0, 0, "Temp Control", SSD1306_COLOR_WHITE);

        // 显示目标温度
        snprintf(buffer, sizeof(buffer), "Target: %.1fC", TempControl_GetTarget());
        SSD1306_WriteString(0, 16, buffer, SSD1306_COLOR_WHITE);

        // 显示当前温度
        snprintf(buffer, sizeof(buffer), "Current:%.1fC", TempControl_GetCurrent());
        SSD1306_WriteString(0, 32, buffer, SSD1306_COLOR_WHITE);

        // 显示系统状态
        const char *state_str = "IDLE";
        switch (TempControl_GetState()) {
            case TEMP_CTRL_HEATING:
                state_str = "HEATING";
                break;
            case TEMP_CTRL_COOLING:
                state_str = "COOLING";
                break;
            case TEMP_CTRL_STABLE:
                state_str = "STABLE";
                break;
            default:
                state_str = "IDLE";
                break;
        }
        snprintf(buffer, sizeof(buffer), "State: %s", state_str);
        SSD1306_WriteString(0, 48, buffer, SSD1306_COLOR_WHITE);

        SSD1306_UpdateScreen();
    }
}

/**
 * @brief  蓝牙接收回调函数
 * @param  data: 接收到的数据
 * @param  len: 数据长度
 */
void SystemTask_BluetoothRxCallback(uint8_t *data, uint16_t len)
{
    char response[64];

    // 解析命令
    if (strncmp((char*)data, BT_CMD_SET_PREFIX, strlen(BT_CMD_SET_PREFIX)) == 0) {
        // SET:XX.X 命令：设置目标温度
        float new_target = atof((char*)data + strlen(BT_CMD_SET_PREFIX));

        if (new_target >= TEMP_MIN && new_target <= TEMP_MAX) {
            TempControl_SetTarget(new_target);
            snprintf(response, sizeof(response), "OK: Target set to %.1f C\r\n", new_target);
        } else {
            snprintf(response, sizeof(response), "ERROR: Temp out of range (%.1f-%.1f)\r\n",
                     TEMP_MIN, TEMP_MAX);
        }

        BSP_Bluetooth_TransmitString(response);

    } else if (strncmp((char*)data, BT_CMD_GET, strlen(BT_CMD_GET)) == 0) {
        // GET 命令：获取当前温度和目标温度
        snprintf(response, sizeof(response), "Target:%.1f Current:%.1f\r\n",
                 TempControl_GetTarget(), TempControl_GetCurrent());
        BSP_Bluetooth_TransmitString(response);

    } else {
        // 未知命令
        BSP_Bluetooth_TransmitString("ERROR: Unknown command\r\n");
        BSP_Bluetooth_TransmitString("Usage: SET:XX.X or GET\r\n");
    }
}
