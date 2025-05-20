#ifndef ENCODER_H
#define ENCODER_H

#include "pico/stdlib.h"

// Направления вращения энкодера
#define ENCODER_DIR_NONE 0
#define ENCODER_DIR_CW   1    // По часовой стрелке
#define ENCODER_DIR_CCW  -1   // Против часовой стрелки

// Пины энкодера
#define ENCODER_PIN_A 16
#define ENCODER_PIN_B 17
#define ENCODER_PIN_BTN 20

// Параметры дебаунсинга
#define DEBOUNCE_DELAY_MS 50  // Задержка для устранения дребезга в миллисекундах

void encoder_init(void);
void encoder_update(void);  // Должна вызываться регулярно для опроса энкодера
int encoder_get_direction(void);
bool encoder_button_pressed(void);  // Возвращает текущее состояние кнопки (true если нажата)
bool encoder_button_event(void);    // Возвращает true если произошло нажатие кнопки

#endif
