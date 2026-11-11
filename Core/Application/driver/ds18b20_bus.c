#include "ds18b20_bus.h"
#include "main.h"

/* Open drain throughout: writing 1 RELEASES the bus, never drives it high.
 * DQ needs an external pull-up to 3.3 V. Do not repurpose HAL's SysTick. */
static void release_bus(void)
{
    DS18B20_DQ_GPIO_Port->BSRR = DS18B20_DQ_Pin;
}

static void pull_low(void)
{
    DS18B20_DQ_GPIO_Port->BSRR = (uint32_t)DS18B20_DQ_Pin << 16U;
}

static uint8_t read_bus(void)
{
    return (DS18B20_DQ_GPIO_Port->IDR & DS18B20_DQ_Pin) != 0U;
}

static void delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = us * (SystemCoreClock / 1000000U);
    while ((uint32_t)(DWT->CYCCNT - start) < cycles) {
        __NOP();
    }
}

void DS18B20_BusInit(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    release_bus();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = DS18B20_DQ_Pin;
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DS18B20_DQ_GPIO_Port, &gpio);
}

bool DS18B20_BusReset(void)
{
    release_bus();
    delay_us(5U);
    if (!read_bus()) return false; /* Shorted/stuck-low is NOT presence. */
    pull_low();
    delay_us(480U);

    /* Only release/sample timing is critical; keep HAL tick interrupts alive. */
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    release_bus();
    delay_us(70U);
    bool present = !read_bus();
    __set_PRIMASK(primask);
    delay_us(410U);
    return present && read_bus();
}

static void write_bit(uint8_t bit)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    pull_low();
    delay_us(bit ? 6U : 60U);
    release_bus();
    delay_us(bit ? 64U : 10U);
    __set_PRIMASK(primask);
}

uint8_t DS18B20_BusReadBit(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    pull_low();
    delay_us(2U);
    release_bus();
    delay_us(10U);
    uint8_t bit = read_bus(); /* Sample within 15 us of the falling edge. */
    delay_us(55U);
    __set_PRIMASK(primask);
    return bit;
}

void DS18B20_BusWriteByte(uint8_t value)
{
    for (unsigned i = 0; i < 8U; ++i) {
        write_bit(value & 1U);
        value >>= 1U;
    }
}

uint8_t DS18B20_BusReadByte(void)
{
    uint8_t value = 0U;
    for (unsigned i = 0; i < 8U; ++i) {
        value |= (uint8_t)(DS18B20_BusReadBit() << i);
    }
    return value;
}
