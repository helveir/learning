#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "dac.h"

// Пины ЦАП в порядке подключения (D9-D0)
const uint8_t dac_pins[] = {23,27,29,28,26,22,21,1,2,3};

// Регистры GPIO для быстрого доступа
static uint32_t* const gpio_set = (uint32_t*)(SIO_BASE + SIO_GPIO_OUT_SET_OFFSET);
static uint32_t* const gpio_clr = (uint32_t*)(SIO_BASE + SIO_GPIO_OUT_CLR_OFFSET);

// Маски для каждого пина ЦАП
static uint32_t pin_masks[10];

void dac_init(void) {
    // Инициализация пинов ЦАП
    for(int i = 0; i < 10; i++) {
        gpio_init(dac_pins[i]);
        gpio_set_dir(dac_pins[i], GPIO_OUT);
        gpio_put(dac_pins[i], 0);
        pin_masks[i] = 1ul << dac_pins[i];
    }
}


void dac_write(uint16_t value) {
    uint32_t set_mask = 0;
    uint32_t clr_mask = 0;
    
    // Формируем маски для установки/сброса битов
    for(int i = 0; i < 10; i++) {
        if(value & (1 << (9-i))) {
            set_mask |= pin_masks[i];
        } else {
            clr_mask |= pin_masks[i];
        }
    }
    
    // Применяем маски одновременно
    *gpio_clr = clr_mask;
    *gpio_set = set_mask;
} 