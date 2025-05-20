#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/gpio.h"
#include "dac.h"
#include "encoder.h"
#include <math.h>

// Прототип функции
uint32_t logic_get_frequency(void);

// Параметры генерации сигнала
#define POINTS_PER_TOOTH 32          // Количество точек на один зуб
#define SIGNAL_LUT_SIZE (60 * POINTS_PER_TOOTH)  // Размер для датчика коленвала (60 зубьев * точек на зуб)
#define CAM_LUT_SIZE 32              // Размер для датчика распредвала
#define ABS_LUT_SIZE 32              // Размер для датчика ABS
#define BASE_AMPLITUDE 127           // Базовая амплитуда сигнала (для 8 бит)
#define CRANK_BASE_AMPLITUDE 32      // Базовая амплитуда для коленвала
#define MAX_AMPLITUDE 127            // Максимальная амплитуда сигнала
#define SIGNAL_OFFSET 128            // Смещение сигнала (для 8 бит)
#define SIGNAL_BITS 8                // Разрядность значений в таблице
#define TOOTH_WIDTH 8                // Ширина зуба в точках
#define GAP_WIDTH 16                 // Ширина пропуска (2 зуба)

// Параметры управления частотой/напряжением
#define TIMER_INTERVAL_MIN 10000    // Минимальный интервал (нс) ~10 Гц макс
#define TIMER_INTERVAL_MAX 1000000  // Максимальный интервал (нс) ~0.1 Гц мин
#define TIMER_INTERVAL_STEP 10000   // Шаг изменения интервала (нс)
#define TIMER_INTERVAL_DEFAULT 100000 // Начальный интервал (нс) ~1 Гц (60 RPM)
#define VOLTAGE_MIN 0                // Минимальное напряжение
#define VOLTAGE_MAX 255              // Максимальное напряжение
#define VOLTAGE_STEP 1               // Шаг изменения напряжения
#define AMPLITUDE_MIN 32             // Минимальная амплитуда (медленное вращение)
#define AMPLITUDE_MAX 127            // Максимальная амплитуда (быстрое вращение)
#define AMPLITUDE_STEP 1             // Шаг изменения амплитуды

// Встроенный светодиод
#define LED_PIN 25

// Типы сигналов
typedef enum {
    WAVE_ABS = 0,        // Датчик ABS
    WAVE_CRANK,          // Датчик коленвала (60-2)
    WAVE_CAM,            // Датчик распредвала
    WAVE_DC_VOLTAGE,     // Регулируемое напряжение
    WAVE_COUNT
} wave_type_t;

static uint8_t abs_lut[ABS_LUT_SIZE];           // Сигнал ABS
static uint8_t crank_lut[SIGNAL_LUT_SIZE];      // Сигнал коленвала
static uint8_t cam_lut[CAM_LUT_SIZE];           // Сигнал распредвала
static uint8_t dc_voltage = SIGNAL_OFFSET;      // Постоянное напряжение
static uint8_t *current_lut = abs_lut;
static int current_index = 0;
static uint32_t timer_interval = TIMER_INTERVAL_DEFAULT;
static wave_type_t current_wave = WAVE_ABS;
static uint8_t current_lut_size = ABS_LUT_SIZE;
static uint8_t crank_amplitude = AMPLITUDE_MIN;  // Амплитуда для датчика коленвала

// Генерация сигналов датчиков
static void generate_sensor_signals() {
    // Генерация сигнала ABS (цифровой)
    uint8_t high_level = (uint8_t)(SIGNAL_OFFSET + BASE_AMPLITUDE);
    uint8_t low_level = (uint8_t)(SIGNAL_OFFSET - BASE_AMPLITUDE);
    
    for (int i = 0; i < ABS_LUT_SIZE; i++) {
        // Генерируем меандр (прямоугольный сигнал)
        abs_lut[i] = (i < ABS_LUT_SIZE/2) ? high_level : low_level;
    }
    
    // Генерация сигнала коленвала (синусоидальный с пропуском)
    for (int i = 0; i < SIGNAL_LUT_SIZE; i++) {
        int current_tooth = i / POINTS_PER_TOOTH;  // Определяем текущий зуб
        
        if (current_tooth >= 58) {
            // Пропуск двух зубьев
            crank_lut[i] = SIGNAL_OFFSET;
        } else {
            // Синусоидальный сигнал
            float angle = 2.0f * M_PI * (i % POINTS_PER_TOOTH) / POINTS_PER_TOOTH;  // Фаза внутри зуба
            crank_lut[i] = (uint8_t)(SIGNAL_OFFSET + CRANK_BASE_AMPLITUDE * sinf(angle));
        }
    }
    
    // Генерация сигнала распредвала (один импульс на оборот)
    for (int i = 0; i < CAM_LUT_SIZE; i++) {
        // Импульс занимает 1/8 периода
        cam_lut[i] = (i < CAM_LUT_SIZE/8) ? high_level : low_level;
    }
}

void logic_init() {
    // Инициализация периферии
    dac_init();
    encoder_init();

    // Инициализация светодиода
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    // Генерация сигналов датчиков
    generate_sensor_signals();
    current_lut = abs_lut;
    current_lut_size = ABS_LUT_SIZE;
}

void logic_update() {
    static uint64_t next_update = 0;
    uint64_t current_time = time_us_64() * 1000;
    
    // Обновляем состояние энкодера
    encoder_update();

    // Управление светодиодом - горит только при нажатой кнопке
    gpio_put(LED_PIN, encoder_button_pressed());
    
    // Переключение типа сигнала по нажатию кнопки
    if (encoder_button_event()) {
        current_wave = (current_wave + 1) % WAVE_COUNT;
        switch(current_wave) {
            case WAVE_ABS:
                current_lut = abs_lut;
                current_lut_size = ABS_LUT_SIZE;
                break;
            case WAVE_CRANK:
                current_lut = crank_lut;
                current_lut_size = SIGNAL_LUT_SIZE;
                break;
            case WAVE_CAM:
                current_lut = cam_lut;
                current_lut_size = CAM_LUT_SIZE;
                break;
            case WAVE_DC_VOLTAGE:
                current_lut_size = 1;
                break;
        }
    }
    
    // Обработка энкодера
    int direction = encoder_get_direction();
    if(direction != ENCODER_DIR_NONE) {
        if(current_wave == WAVE_DC_VOLTAGE) {
            // Регулировка напряжения
            if(direction == ENCODER_DIR_CW && dc_voltage < VOLTAGE_MAX) {
                dc_voltage += VOLTAGE_STEP;
            } else if(direction == ENCODER_DIR_CCW && dc_voltage > VOLTAGE_MIN) {
                dc_voltage -= VOLTAGE_STEP;
            }
        } else if(current_wave == WAVE_CRANK) {
            // Для коленвала регулируем амплитуду вместо частоты
            if(direction == ENCODER_DIR_CW && crank_amplitude < AMPLITUDE_MAX) {
                crank_amplitude += AMPLITUDE_STEP;
            } else if(direction == ENCODER_DIR_CCW && crank_amplitude > AMPLITUDE_MIN) {
                crank_amplitude -= AMPLITUDE_STEP;
            }
        } else {
            // Для остальных сигналов регулируем частоту
            if(direction == ENCODER_DIR_CW && timer_interval > TIMER_INTERVAL_MIN) {
                timer_interval -= TIMER_INTERVAL_STEP;
            } else if(direction == ENCODER_DIR_CCW && timer_interval < TIMER_INTERVAL_MAX) {
                timer_interval += TIMER_INTERVAL_STEP;
            }
        }
    }
    
    // Генерация сигнала
    if (current_time >= next_update) {
        if(current_wave == WAVE_DC_VOLTAGE) {
            dac_write(dc_voltage << 2);  // Постоянное напряжение
        } else if(current_wave == WAVE_CRANK) {
            // Для коленвала масштабируем амплитуду, сохраняя фазу
            uint8_t base_value = crank_lut[current_index];
            if(base_value != SIGNAL_OFFSET) {  // Не масштабируем в зоне пропуска
                int32_t centered_value = base_value - SIGNAL_OFFSET;
                int32_t scaled_value = (centered_value * crank_amplitude) / CRANK_BASE_AMPLITUDE;
                base_value = (uint8_t)(SIGNAL_OFFSET + scaled_value);
            }
            dac_write(base_value << 2);
            current_index = (current_index + 1) % current_lut_size;
        } else {
            dac_write(current_lut[current_index] << 2);
            current_index = (current_index + 1) % current_lut_size;
        }
        next_update = current_time + timer_interval;
    }
}

// Получение текущей частоты в Гц
uint32_t logic_get_frequency(void) {
    if(current_wave == WAVE_DC_VOLTAGE) {
        return 0;  // Для DC напряжения частота не имеет смысла
    }
    // Частота = 1 / (период * количество точек)
    return (uint32_t)(1.0e9f / (timer_interval * current_lut_size));
} 