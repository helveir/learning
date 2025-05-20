#include "pico/stdlib.h"
#include "encoder.h"
#include "hardware/gpio.h"

static int last_a = 0;
static int last_b = 0;
static bool last_button = true;  // true = не нажата (подтянута к питанию)
static bool button_pressed_flag = false;
static bool button_event_flag = false;
static absolute_time_t last_button_time;
int encoder_direction = 0;

void encoder_init() {
    gpio_init(ENCODER_PIN_A);
    gpio_init(ENCODER_PIN_B);
    gpio_init(ENCODER_PIN_BTN);

    gpio_set_dir(ENCODER_PIN_A, GPIO_IN);
    gpio_set_dir(ENCODER_PIN_B, GPIO_IN);
    gpio_set_dir(ENCODER_PIN_BTN, GPIO_IN);

    gpio_pull_up(ENCODER_PIN_A);
    gpio_pull_up(ENCODER_PIN_B);
    gpio_pull_up(ENCODER_PIN_BTN);  // Подтягиваем кнопку к питанию
    
    last_a = gpio_get(ENCODER_PIN_A);
    last_b = gpio_get(ENCODER_PIN_B);
    last_button = gpio_get(ENCODER_PIN_BTN);
    last_button_time = get_absolute_time();
}

void encoder_update() {
    // Обработка энкодера
    int current_a = gpio_get(ENCODER_PIN_A);
    int current_b = gpio_get(ENCODER_PIN_B);
    
    // Определяем направление по изменению состояния пина A
    if (current_a != last_a) {  // Если изменился пин A
        encoder_direction = (current_a == current_b) ? -1 : 1;
    }
    
    last_a = current_a;
    last_b = current_b;

    // Обработка кнопки с устранением дребезга
    bool current_button = gpio_get(ENCODER_PIN_BTN);
    absolute_time_t current_time = get_absolute_time();
    
    if (current_button != last_button && 
        absolute_time_diff_us(last_button_time, current_time) > DEBOUNCE_DELAY_MS * 1000) {
        
        if (last_button && !current_button) {  // Фронт нажатия (с high на low)
            button_event_flag = true;
        }
        
        last_button = current_button;
        last_button_time = current_time;
    }
    
    button_pressed_flag = !current_button;  // Инвертируем т.к. кнопка подтянута к питанию
}

bool encoder_button_pressed() {
    return button_pressed_flag;
}

bool encoder_button_event() {
    bool event = button_event_flag;
    button_event_flag = false;  // Сбрасываем флаг после чтения
    return event;
}

int encoder_get_direction() {
    int dir = encoder_direction;
    encoder_direction = 0;
    return dir;
}
