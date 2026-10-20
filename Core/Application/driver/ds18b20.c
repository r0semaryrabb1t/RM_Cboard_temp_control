/**
 ******************************************************************************
 * @file           : ds18b20.c
 * @brief          : DS18B20 温度传感器驱动实现
 ******************************************************************************
 */

#include "ds18b20.h"
#include "robot_config.h"
#include "gpio.h"

/* DS18B20 DQ 引脚操作宏 */
#define DS18B20_DQ_PIN      GPIO_PIN_12
#define DS18B20_DQ_PORT     GPIOA

#define DS18B20_DQ_OUT(x)   HAL_GPIO_WritePin(DS18B20_DQ_PORT, DS18B20_DQ_PIN, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define DS18B20_DQ_IN()     HAL_GPIO_ReadPin(DS18B20_DQ_PORT, DS18B20_DQ_PIN)

/* 微秒延时（粗略实现，需要根据实际时钟调整） */
static void delay_us(uint32_t us)
{
    uint32_t ticks = us * (SystemCoreClock / 1000000) / 5;
    while (ticks--);
}

/* 设置 DQ 引脚为输出模式 */
static void DS18B20_SetOutputMode(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS18B20_DQ_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DS18B20_DQ_PORT, &GPIO_InitStruct);
}

/* 设置 DQ 引脚为输入模式 */
static void DS18B20_SetInputMode(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS18B20_DQ_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DS18B20_DQ_PORT, &GPIO_InitStruct);
}

/* 复位 DS18B20 */
static bool DS18B20_Reset(void)
{
    bool present = false;

    DS18B20_SetOutputMode();
    DS18B20_DQ_OUT(0);
    delay_us(DS18B20_RESET_DELAY_US);

    DS18B20_DQ_OUT(1);
    DS18B20_SetInputMode();
    delay_us(DS18B20_PRESENCE_DELAY_US);

    if (DS18B20_DQ_IN() == GPIO_PIN_RESET) {
        present = true;
    }

    delay_us(DS18B20_RESET_DELAY_US - DS18B20_PRESENCE_DELAY_US);

    return present;
}

/* 写一个位 */
static void DS18B20_WriteBit(uint8_t bit)
{
    DS18B20_SetOutputMode();
    DS18B20_DQ_OUT(0);

    if (bit) {
        delay_us(DS18B20_WRITE_1_DELAY_US);
        DS18B20_DQ_OUT(1);
        delay_us(DS18B20_WRITE_0_DELAY_US - DS18B20_WRITE_1_DELAY_US);
    } else {
        delay_us(DS18B20_WRITE_0_DELAY_US);
        DS18B20_DQ_OUT(1);
    }

    delay_us(5);
}

/* 读一个位 */
static uint8_t DS18B20_ReadBit(void)
{
    uint8_t bit = 0;

    DS18B20_SetOutputMode();
    DS18B20_DQ_OUT(0);
    delay_us(2);

    DS18B20_DQ_OUT(1);
    DS18B20_SetInputMode();
    delay_us(DS18B20_READ_DELAY_US);

    if (DS18B20_DQ_IN() == GPIO_PIN_SET) {
        bit = 1;
    }

    delay_us(DS18B20_WRITE_0_DELAY_US - DS18B20_READ_DELAY_US);

    return bit;
}

/* 写一个字节 */
static void DS18B20_WriteByte(uint8_t byte)
{
    for (uint8_t i = 0; i < 8; i++) {
        DS18B20_WriteBit(byte & 0x01);
        byte >>= 1;
    }
}

/* 读一个字节 */
static uint8_t DS18B20_ReadByte(void)
{
    uint8_t byte = 0;

    for (uint8_t i = 0; i < 8; i++) {
        byte >>= 1;
        if (DS18B20_ReadBit()) {
            byte |= 0x80;
        }
    }

    return byte;
}

/**
 * @brief  初始化 DS18B20
 */
void DS18B20_Init(void)
{
    DS18B20_SetOutputMode();
    DS18B20_DQ_OUT(1);
}

/**
 * @brief  检测 DS18B20 是否存在
 */
bool DS18B20_IsPresent(void)
{
    return DS18B20_Reset();
}

/**
 * @brief  启动温度转换
 */
void DS18B20_StartConversion(void)
{
    DS18B20_Reset();
    DS18B20_WriteByte(DS18B20_CMD_SKIP_ROM);
    DS18B20_WriteByte(DS18B20_CMD_CONVERT_T);
}

/**
 * @brief  读取温度（阻塞式）
 * @return 温度值（摄氏度）
 */
float DS18B20_ReadTemperature(void)
{
    uint8_t temp_lsb, temp_msb;
    int16_t temp_raw;
    float temperature;

    // 启动转换
    DS18B20_StartConversion();

    // 等待转换完成
    HAL_Delay(DS18B20_CONVERT_TIME_MS);

    // 读取温度数据
    DS18B20_Reset();
    DS18B20_WriteByte(DS18B20_CMD_SKIP_ROM);
    DS18B20_WriteByte(DS18B20_CMD_READ_SCRATCHPAD);

    temp_lsb = DS18B20_ReadByte();
    temp_msb = DS18B20_ReadByte();

    temp_raw = (int16_t)((temp_msb << 8) | temp_lsb);
    temperature = (float)temp_raw / 16.0f;

    return temperature;
}

/**
 * @brief  读取温度（非阻塞式，需要外部状态机配合）
 * @param  temp: 温度输出指针
 * @return true 表示读取成功，false 表示需要等待转换完成
 */
bool DS18B20_ReadTemperatureNonBlocking(float *temp)
{
    static enum {
        STATE_START_CONVERSION,
        STATE_WAIT_CONVERSION,
        STATE_READ_DATA
    } state = STATE_START_CONVERSION;

    static uint32_t conversion_start_tick = 0;

    switch (state) {
        case STATE_START_CONVERSION:
            DS18B20_StartConversion();
            conversion_start_tick = HAL_GetTick();
            state = STATE_WAIT_CONVERSION;
            return false;

        case STATE_WAIT_CONVERSION:
            if (HAL_GetTick() - conversion_start_tick >= DS18B20_CONVERT_TIME_MS) {
                state = STATE_READ_DATA;
            }
            return false;

        case STATE_READ_DATA: {
            uint8_t temp_lsb, temp_msb;
            int16_t temp_raw;

            DS18B20_Reset();
            DS18B20_WriteByte(DS18B20_CMD_SKIP_ROM);
            DS18B20_WriteByte(DS18B20_CMD_READ_SCRATCHPAD);

            temp_lsb = DS18B20_ReadByte();
            temp_msb = DS18B20_ReadByte();

            temp_raw = (int16_t)((temp_msb << 8) | temp_lsb);
            *temp = (float)temp_raw / 16.0f;

            state = STATE_START_CONVERSION;
            return true;
        }
    }

    return false;
}
