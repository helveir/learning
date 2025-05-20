#include "pico/stdlib.h"
#include "encoder.h"
#include "logic.h"

int main() {
    // Инициализация UART для отладки
    if (!uart_init(uart0, 115200)) {
    }
    
    // Инициализация стандартного ввода/вывода
    stdio_init_all();
    
    // Инициализация основной логики (включает инициализацию энкодера и ЦАП)
    logic_init();

    // Главный цикл
    while (true) {
        logic_update();    // Обработка логики и генерация сигнала
        // tight_loop_contents();  // Оптимизация компилятором
    }
}
