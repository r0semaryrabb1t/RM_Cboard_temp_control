/**
 ******************************************************************************
 * @file           : ssd1306.h
 * @brief          : SSD1306 OLED 显示屏驱动（I2C接口）
 ******************************************************************************
 */

#ifndef __SSD1306_H
#define __SSD1306_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/* 显示屏尺寸 */
#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT  64

/* 颜色定义 */
typedef enum {
    SSD1306_COLOR_BLACK = 0,
    SSD1306_COLOR_WHITE = 1
} SSD1306_Color_t;

/* 函数声明 */
bool SSD1306_Init(void);
void SSD1306_UpdateScreen(void);
void SSD1306_Fill(SSD1306_Color_t color);
void SSD1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_Color_t color);
void SSD1306_WriteChar(uint8_t x, uint8_t y, char ch, SSD1306_Color_t color);
void SSD1306_WriteString(uint8_t x, uint8_t y, const char *str, SSD1306_Color_t color);
void SSD1306_SetCursor(uint8_t x, uint8_t y);
void SSD1306_Clear(void);

#ifdef __cplusplus
}
#endif

#endif /* __SSD1306_H */
