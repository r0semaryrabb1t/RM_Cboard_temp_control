#include "system_task.h"
#include "robot_config.h"
#include "temp_control.h"
#include "ds18b20.h"
#include "ssd1306.h"
#include "i2c.h"
#include "bsp_pwm.h"
#include <stdio.h>

#ifndef TEMP_OLED_INVERT_TEST
#define TEMP_OLED_INVERT_TEST 0
#endif

static uint32_t last_display_tick;
static uint32_t last_oled_retry_tick;
static bool oled_ready;
#if TEMP_OLED_INVERT_TEST
static uint32_t oled_test_tick;
static bool oled_test_white;
#endif

/* newlib-nano does not link float printf by default. Format tenths as integers. */
static void format_temperature(char *buffer, size_t size,
                               const char *label, float temperature)
{
    int tenths = (int)(temperature * 10.0f + (temperature >= 0.0f ? 0.5f : -0.5f));
    unsigned magnitude = (unsigned)(tenths < 0 ? -tenths : tenths);
    snprintf(buffer, size, "%s:%s%u.%uC", label, tenths < 0 ? "-" : "",
             magnitude / 10U, magnitude % 10U);
}

static void display_temperature(void)
{
    char line[22];
    SSD1306_Fill(SSD1306_COLOR_BLACK);
    bool temperature_valid = TempControl_IsTemperatureValid();
    float current_temperature = TempControl_GetCurrent();
    DS18B20_Status_t sensor_status = DS18B20_GetStatus();
    if (temperature_valid) {
        format_temperature(line, sizeof(line), "CURR", current_temperature);
    } else {
        const char *error;
        switch (sensor_status) {
            case DS18B20_WAITING:      error = "WAIT"; break;
            case DS18B20_NO_DEVICE:    error = "NO SENSOR"; break;
            case DS18B20_CRC_ERROR:    error = "CRC ERR"; break;
            case DS18B20_TIMEOUT:      error = "TIMEOUT"; break;
            default:                  error = "DATA ERR"; break;
        }
        snprintf(line, sizeof(line), "CURR:%s", error);
    }
    SSD1306_WriteString(0, 16, line, SSD1306_COLOR_WHITE);
    format_temperature(line, sizeof(line), "SET", TempControl_GetTarget());
    SSD1306_WriteString(0, 40, line, SSD1306_COLOR_WHITE);
    oled_ready = SSD1306_UpdateScreen();
}


void SystemTask_Init(void)
{
    TempControl_Init();
#if TEMP_OLED_INVERT_TEST
    /* Keep heater and both fans off while exercising the OLED panel. */
    TempControl_Enable(false);
    BSP_PWM_SetDutyCycle(PWM_CH_HEATER, 0U);
    BSP_PWM_SetDutyCycle(PWM_CH_FAN1, 0U);
    BSP_PWM_SetDutyCycle(PWM_CH_FAN2, 0U);
#else
    TempControl_Enable(TEMP_CONTROL_ENABLED != 0);
    /* PID may command the heater later; start its timer only in control mode. */
    BSP_PWM_Start(PWM_CH_HEATER);
#endif
    /* Start first DS18B20 conversion immediately, then keep the display live. */
    TempControl_Update();
    oled_ready = SSD1306_Init();
    if (oled_ready) {
#if TEMP_OLED_INVERT_TEST
        SSD1306_Fill(SSD1306_COLOR_BLACK);
        oled_ready = SSD1306_UpdateScreen();
#else
        display_temperature();
#endif
    }
    last_display_tick = last_oled_retry_tick = HAL_GetTick();
#if TEMP_OLED_INVERT_TEST
    oled_test_tick = last_display_tick;
#endif
}

void SystemTask_Run(void)
{
    TempControl_Update();
    uint32_t now = HAL_GetTick();
#if TEMP_OLED_INVERT_TEST
    if (oled_ready && (uint32_t)(now - oled_test_tick) >= 1000U) {
        oled_test_tick = now;
        oled_test_white = !oled_test_white;
        SSD1306_Fill(oled_test_white ? SSD1306_COLOR_WHITE : SSD1306_COLOR_BLACK);
        oled_ready = SSD1306_UpdateScreen();
    }
#endif
    if (!oled_ready && (uint32_t)(now - last_oled_retry_tick) >= 1000U) {
        last_oled_retry_tick = now;
        oled_ready = (I2C1_RecoverBus() == HAL_OK) && SSD1306_Init();
    }
#if !TEMP_OLED_INVERT_TEST
    if (oled_ready && (uint32_t)(now - last_display_tick) >= DISPLAY_UPDATE_PERIOD_MS) {
        last_display_tick = now;
        display_temperature();
    }
#endif
}
