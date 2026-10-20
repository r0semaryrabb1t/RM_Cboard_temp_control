/**
 ******************************************************************************
 * @file           : ssd1306.c
 * @brief          : SSD1306 OLED 显示屏驱动实现
 ******************************************************************************
 */

#include "ssd1306.h"
#include "robot_config.h"
#include "main.h"
#include <string.h>

/* 外部 I2C 句柄 (定义在 main.c) */
extern I2C_HandleTypeDef hi2c1;

/* SSD1306 命令 */
#define SSD1306_CMD_DISPLAY_OFF         0xAE
#define SSD1306_CMD_DISPLAY_ON          0xAF
#define SSD1306_CMD_SET_CONTRAST        0x81
#define SSD1306_CMD_NORMAL_DISPLAY      0xA6
#define SSD1306_CMD_INVERSE_DISPLAY     0xA7
#define SSD1306_CMD_SET_MULTIPLEX       0xA8
#define SSD1306_CMD_SET_DISPLAY_OFFSET  0xD3
#define SSD1306_CMD_SET_START_LINE      0x40
#define SSD1306_CMD_CHARGE_PUMP         0x8D
#define SSD1306_CMD_MEMORY_MODE         0x20
#define SSD1306_CMD_SEG_REMAP           0xA1
#define SSD1306_CMD_COM_SCAN_DEC        0xC8
#define SSD1306_CMD_SET_COM_PINS        0xDA
#define SSD1306_CMD_SET_PRECHARGE       0xD9
#define SSD1306_CMD_SET_VCOM_DETECT     0xDB
#define SSD1306_CMD_SET_CLOCK_DIV       0xD5

/* 显示缓冲区 */
static uint8_t ssd1306_buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

/* 当前光标位置 */
static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;

/* 5x7 ASCII 字体 (字符 32-126) */
static const uint8_t Font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // ' '
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // '!'
    {0x00, 0x07, 0x00, 0x07, 0x00}, // '"'
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // '#'
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // '$'
    {0x23, 0x13, 0x08, 0x64, 0x62}, // '%'
    {0x36, 0x49, 0x55, 0x22, 0x50}, // '&'
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '''
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // '('
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // ')'
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // '*'
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // '+'
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ','
    {0x08, 0x08, 0x08, 0x08, 0x08}, // '-'
    {0x00, 0x60, 0x60, 0x00, 0x00}, // '.'
    {0x20, 0x10, 0x08, 0x04, 0x02}, // '/'
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // '0'
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // '1'
    {0x42, 0x61, 0x51, 0x49, 0x46}, // '2'
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // '3'
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // '4'
    {0x27, 0x45, 0x45, 0x45, 0x39}, // '5'
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // '6'
    {0x01, 0x71, 0x09, 0x05, 0x03}, // '7'
    {0x36, 0x49, 0x49, 0x49, 0x36}, // '8'
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // '9'
    {0x00, 0x36, 0x36, 0x00, 0x00}, // ':'
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ';'
    {0x08, 0x14, 0x22, 0x41, 0x00}, // '<'
    {0x14, 0x14, 0x14, 0x14, 0x14}, // '='
    {0x00, 0x41, 0x22, 0x14, 0x08}, // '>'
    {0x02, 0x01, 0x51, 0x09, 0x06}, // '?'
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // '@'
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 'A'
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 'B'
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 'C'
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 'D'
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 'E'
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 'F'
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 'G'
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 'H'
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 'I'
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 'J'
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 'K'
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 'L'
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 'M'
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 'N'
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 'O'
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 'P'
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 'Q'
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 'R'
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 'S'
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 'T'
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 'U'
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 'V'
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // 'W'
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 'X'
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 'Y'
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 'Z'
};

/* 写命令 */
static void SSD1306_WriteCommand(uint8_t cmd)
{
    HAL_I2C_Mem_Write(&hi2c1, OLED_I2C_ADDRESS, 0x00, 1, &cmd, 1, HAL_MAX_DELAY);
}

/* 写数据 */
static void SSD1306_WriteData(uint8_t *data, uint16_t len)
{
    HAL_I2C_Mem_Write(&hi2c1, OLED_I2C_ADDRESS, 0x40, 1, data, len, HAL_MAX_DELAY);
}

/**
 * @brief  初始化 SSD1306
 */
bool SSD1306_Init(void)
{
    HAL_Delay(100);  // 等待 OLED 上电稳定

    SSD1306_WriteCommand(SSD1306_CMD_DISPLAY_OFF);
    SSD1306_WriteCommand(SSD1306_CMD_SET_CLOCK_DIV);
    SSD1306_WriteCommand(0x80);
    SSD1306_WriteCommand(SSD1306_CMD_SET_MULTIPLEX);
    SSD1306_WriteCommand(SSD1306_HEIGHT - 1);
    SSD1306_WriteCommand(SSD1306_CMD_SET_DISPLAY_OFFSET);
    SSD1306_WriteCommand(0x00);
    SSD1306_WriteCommand(SSD1306_CMD_SET_START_LINE | 0x00);
    SSD1306_WriteCommand(SSD1306_CMD_CHARGE_PUMP);
    SSD1306_WriteCommand(0x14);
    SSD1306_WriteCommand(SSD1306_CMD_MEMORY_MODE);
    SSD1306_WriteCommand(0x00);
    SSD1306_WriteCommand(SSD1306_CMD_SEG_REMAP);
    SSD1306_WriteCommand(SSD1306_CMD_COM_SCAN_DEC);
    SSD1306_WriteCommand(SSD1306_CMD_SET_COM_PINS);
    SSD1306_WriteCommand(0x12);
    SSD1306_WriteCommand(SSD1306_CMD_SET_CONTRAST);
    SSD1306_WriteCommand(0xCF);
    SSD1306_WriteCommand(SSD1306_CMD_SET_PRECHARGE);
    SSD1306_WriteCommand(0xF1);
    SSD1306_WriteCommand(SSD1306_CMD_SET_VCOM_DETECT);
    SSD1306_WriteCommand(0x40);
    SSD1306_WriteCommand(SSD1306_CMD_NORMAL_DISPLAY);
    SSD1306_WriteCommand(SSD1306_CMD_DISPLAY_ON);

    SSD1306_Fill(SSD1306_COLOR_BLACK);
    SSD1306_UpdateScreen();

    return true;
}

/**
 * @brief  更新屏幕显示
 */
void SSD1306_UpdateScreen(void)
{
    for (uint8_t i = 0; i < 8; i++) {
        SSD1306_WriteCommand(0xB0 + i);  // 设置页地址
        SSD1306_WriteCommand(0x00);      // 设置列低地址
        SSD1306_WriteCommand(0x10);      // 设置列高地址
        SSD1306_WriteData(&ssd1306_buffer[SSD1306_WIDTH * i], SSD1306_WIDTH);
    }
}

/**
 * @brief  填充整个屏幕
 */
void SSD1306_Fill(SSD1306_Color_t color)
{
    memset(ssd1306_buffer, (color == SSD1306_COLOR_BLACK) ? 0x00 : 0xFF, sizeof(ssd1306_buffer));
}

/**
 * @brief  画一个像素点
 */
void SSD1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_Color_t color)
{
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;

    if (color == SSD1306_COLOR_WHITE) {
        ssd1306_buffer[x + (y / 8) * SSD1306_WIDTH] |= (1 << (y % 8));
    } else {
        ssd1306_buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
    }
}

/**
 * @brief  写一个字符
 */
void SSD1306_WriteChar(uint8_t x, uint8_t y, char ch, SSD1306_Color_t color)
{
    if (ch < 32 || ch > 126) return;

    uint8_t font_index = ch - 32;

    for (uint8_t i = 0; i < 5; i++) {
        uint8_t line = Font5x7[font_index][i];
        for (uint8_t j = 0; j < 8; j++) {
            if (line & (1 << j)) {
                SSD1306_DrawPixel(x + i, y + j, color);
            } else {
                SSD1306_DrawPixel(x + i, y + j, !color);
            }
        }
    }
}

/**
 * @brief  写字符串
 */
void SSD1306_WriteString(uint8_t x, uint8_t y, const char *str, SSD1306_Color_t color)
{
    while (*str) {
        if (x + 6 > SSD1306_WIDTH) {
            x = 0;
            y += 8;
            if (y + 8 > SSD1306_HEIGHT) break;
        }

        SSD1306_WriteChar(x, y, *str, color);
        x += 6;
        str++;
    }
}

/**
 * @brief  设置光标位置
 */
void SSD1306_SetCursor(uint8_t x, uint8_t y)
{
    cursor_x = x;
    cursor_y = y;
}

/**
 * @brief  清屏
 */
void SSD1306_Clear(void)
{
    SSD1306_Fill(SSD1306_COLOR_BLACK);
    SSD1306_SetCursor(0, 0);
}
