#ifndef DAC_H
#define DAC_H

#include "pico/stdlib.h"

// Параметры ЦАП
#define DAC_BITS 10                    // Разрядность ЦАП
#define DAC_MAX_VALUE ((1 << DAC_BITS) - 1)  // Максимальное значение (1023 для 10 бит)
#define DAC_PINS_COUNT DAC_BITS        // Количество пинов ЦАП

// Инициализация ЦАП
void dac_init(void);

// Запись значения в ЦАП (0-1023 для 10 бит)
void dac_write(uint16_t value);

#endif // DAC_H 