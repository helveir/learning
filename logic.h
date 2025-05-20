#ifndef LOGIC_H
#define LOGIC_H

#include <stdint.h>

// Типы сигналов
typedef enum {
    WAVE_SINE = 0,
    WAVE_TRIANGLE,
    WAVE_SAWTOOTH,
    WAVE_SQUARE,
    WAVE_COUNT
} wave_type_t;

// Инициализация логики
void logic_init(void);

// Обновление состояния (вызывается в главном цикле)
void logic_update(void);

// Получение текущей частоты в Гц
uint32_t logic_get_frequency(void);

#endif
