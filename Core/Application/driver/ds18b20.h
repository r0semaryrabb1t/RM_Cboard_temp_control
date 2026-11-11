#ifndef DS18B20_H
#define DS18B20_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DS18B20_WAITING = 0,
    DS18B20_READY,
    DS18B20_NO_DEVICE,
    DS18B20_CRC_ERROR,
    DS18B20_TIMEOUT,
    DS18B20_INVALID_DATA
} DS18B20_Status_t;

void DS18B20_Init(void);
/* Single owner: call frequently from TempControl_Update only.
 * READY writes a fresh, validated sample; all other results leave *temp intact.
 * Uses SKIP ROM: one sensor only, VDD required (no parasite power). */
DS18B20_Status_t DS18B20_Poll(float *temp);
DS18B20_Status_t DS18B20_GetStatus(void);

#ifdef __cplusplus
}
#endif
#endif
