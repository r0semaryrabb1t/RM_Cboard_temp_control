#ifndef DS18B20_BUS_H
#define DS18B20_BUS_H

#include <stdbool.h>
#include <stdint.h>

/* One externally powered DS18B20 on the board's DQ pin. */
void DS18B20_BusInit(void);
bool DS18B20_BusReset(void);
void DS18B20_BusWriteByte(uint8_t value);
uint8_t DS18B20_BusReadByte(void);
uint8_t DS18B20_BusReadBit(void);

#endif
