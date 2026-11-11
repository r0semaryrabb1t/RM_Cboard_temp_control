#include "ds18b20.h"
#include "ds18b20_bus.h"
#include "main.h"
#include "robot_config.h"
#include <stdbool.h>
#include <stddef.h>

#define CMD_SKIP_ROM        0xCCU
#define CMD_CONVERT_T       0x44U
#define CMD_READ_SCRATCHPAD  0xBEU

static enum { START, CONVERTING, RETRY } phase;
static uint32_t phase_tick;
static DS18B20_Status_t last_status;

static uint8_t crc8(const uint8_t *data, unsigned count)
{
    uint8_t crc = 0U;
    for (unsigned i = 0; i < count; ++i) {
        uint8_t value = data[i];
        for (unsigned bit = 0; bit < 8U; ++bit) {
            uint8_t mix = (crc ^ value) & 1U;
            crc >>= 1U;
            if (mix) crc ^= 0x8CU;
            value >>= 1U;
        }
    }
    return crc;
}

static DS18B20_Status_t fail(DS18B20_Status_t status)
{
    last_status = status;
    phase = RETRY;
    phase_tick = HAL_GetTick();
    return status;
}

void DS18B20_Init(void)
{
    DS18B20_BusInit();
    phase = START;
    phase_tick = 0U;
    last_status = DS18B20_WAITING;
}

DS18B20_Status_t DS18B20_GetStatus(void)
{
    return last_status;
}

DS18B20_Status_t DS18B20_Poll(float *temp)
{
    if (temp == NULL) return fail(DS18B20_INVALID_DATA);
    uint32_t now = HAL_GetTick();
    if (phase == RETRY) {
        if ((uint32_t)(now - phase_tick) < DS18B20_RETRY_MS)
            return DS18B20_WAITING;
        phase = START;
    }
    if (phase == START) {
        if (!DS18B20_BusReset()) return fail(DS18B20_NO_DEVICE);
        DS18B20_BusWriteByte(CMD_SKIP_ROM);
        DS18B20_BusWriteByte(CMD_CONVERT_T);
        phase_tick = HAL_GetTick();
        phase = CONVERTING;
        return DS18B20_WAITING;
    }
    uint32_t elapsed = (uint32_t)(now - phase_tick);
    if (elapsed < DS18B20_CONVERT_TIME_MS) return DS18B20_WAITING;
    /* Do not feed an old conversion to PID after a long main-loop stall. */
    if (elapsed >= TEMP_STALE_TIMEOUT_MS || !DS18B20_BusReadBit())
        return fail(DS18B20_TIMEOUT);
    if (!DS18B20_BusReset()) return fail(DS18B20_NO_DEVICE);
    DS18B20_BusWriteByte(CMD_SKIP_ROM);
    DS18B20_BusWriteByte(CMD_READ_SCRATCHPAD);
    uint8_t scratchpad[9];
    bool all_zero = true;
    for (unsigned i = 0; i < sizeof(scratchpad); ++i) {
        scratchpad[i] = DS18B20_BusReadByte();
        if (scratchpad[i] != 0U) all_zero = false;
    }
    /* All-zero data has a valid zero CRC but is not a valid scratchpad. */
    if (all_zero) return fail(DS18B20_INVALID_DATA);
    if (crc8(scratchpad, 8U) != scratchpad[8])
        return fail(DS18B20_CRC_ERROR);
    /* DS18B20 config: bit 7 = 0, bits 4..0 = 1. */
    if ((scratchpad[4] & 0x9FU) != 0x1FU)
        return fail(DS18B20_INVALID_DATA);
    uint16_t raw = ((uint16_t)scratchpad[1] << 8U) | scratchpad[0];
    unsigned resolution = (scratchpad[4] >> 5U) & 3U;
    raw &= (uint16_t)(0xFFFFU << (3U - resolution));
    int32_t signed_raw = (raw & 0x8000U) ? (int32_t)raw - 65536 : raw;
    float value = (float)signed_raw / 16.0f;
    if (value < -55.0f || value > 125.0f)
        return fail(DS18B20_INVALID_DATA);
    *temp = value;
    phase = START;
    last_status = DS18B20_READY;
    return DS18B20_READY;
}
