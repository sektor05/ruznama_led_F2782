#include "display_timer.h"
#include "seven_segment.h"
#include "top_board.h"

// ============================================
// Аппаратные таймеры ESP32 для мультиплексирования
// ============================================
// Используем FreeRTOS задачи с высоким приоритетом для мультиплексирования
// 
// ESP32 имеет два ядра:
// - Core 0: WiFi, TCP/IP, системные задачи
// - Core 1: Arduino loop/setup, пользовательский код
//
// Для стабильной работы динамической индикации независимо от WiFi:
// - Используем отдельные FreeRTOS задачи с высоким приоритетом
// - Закрепляем задачи на ядро 1 (пользовательское ядро)
// - Приоритет выше стандартных задач (configMAX_PRIORITIES-2)

#if defined(ESP32)
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ============================================
// Глобальные переменные задачи нижней платы
// ============================================
static TaskHandle_t bottomBoardTaskHandle = nullptr;
static volatile bool bottomTaskRunning = false;
static volatile bool bottomTaskShouldRun = false;

// ============================================
// Глобальные переменные задачи верхней платы
// ============================================
static TaskHandle_t topBoardTaskHandle = nullptr;
static volatile bool topTaskRunning = false;
static volatile bool topTaskShouldRun = false;

// ============================================
// Задача мультиплексирования нижней платы
// ============================================
// Выполняется на ядре 1 с высоким приоритетом
// Не зависит от работы WiFi на ядре 0
static void bottomBoardTask(void* pvParameters) {
    const TickType_t taskDelay = pdMS_TO_TICKS(TIMER_BOTTOM_INTERVAL_MS);
    
    while (true) {
        if (bottomTaskShouldRun) {
            // Мультиплексирование нижней платы (намазы)
            multiplexDisplay(BOARD_BOTTOM);
        }
        
        // Задержка для стабильной частоты обновления
        vTaskDelay(taskDelay);
    }
}

// ============================================
// Задача мультиплексирования верхней платы
// ============================================
// Выполняется на ядре 1 с высоким приоритетом
// Не зависит от работы WiFi на ядре 0
static void topBoardTask(void* pvParameters) {
    const TickType_t taskDelay = pdMS_TO_TICKS(TIMER_TOP_INTERVAL_MS);
    
    while (true) {
        if (topTaskShouldRun) {
            // Мультиплексирование верхней платы (время/дата)
            multiplexTopBoard();
        }
        
        // Задержка для стабильной частоты обновления
        vTaskDelay(taskDelay);
    }
}

// ============================================
// Инициализация задачи нижней платы
// ============================================
void bottomBoardTimerInit() {
    if (bottomTaskRunning) {
        return;  // Уже инициализирован
    }
    
    // Создаём задачу с высоким приоритетом на ядре 1
    // Приоритет: configMAX_PRIORITIES-2 (высокий, но ниже самых критичных системных)
    BaseType_t ret = xTaskCreatePinnedToCore(
        bottomBoardTask,              // Функция задачи
        "bottom_board_mux",            // Имя задачи
        2048,                          // Размер стека (в словах)
        nullptr,                       // Параметры задачи
        configMAX_PRIORITIES - 2,      // Приоритет (высокий)
        &bottomBoardTaskHandle,        // Handle задачи
        1                              // Ядро 1 (пользовательское)
    );
    
    if (ret != pdPASS) {
        Serial.printf("Ошибка создания задачи нижней платы: %d\n", ret);
        return;
    }
    
    bottomTaskRunning = true;
    Serial.println("Задача нижней платы инициализирована (ядро 1, высокий приоритет)");
}

// ============================================
// Инициализация задачи верхней платы
// ============================================
void topBoardTimerInit() {
    if (topTaskRunning) {
        return;  // Уже инициализирован
    }
    
    // Создаём задачу с высоким приоритетом на ядре 1
    // Приоритет: configMAX_PRIORITIES-2 (высокий, но ниже самых критичных системных)
    BaseType_t ret = xTaskCreatePinnedToCore(
        topBoardTask,                  // Функция задачи
        "top_board_mux",               // Имя задачи
        2048,                          // Размер стека (в словах)
        nullptr,                       // Параметры задачи
        configMAX_PRIORITIES - 2,      // Приоритет (высокий)
        &topBoardTaskHandle,           // Handle задачи
        1                              // Ядро 1 (пользовательское)
    );
    
    if (ret != pdPASS) {
        Serial.printf("Ошибка создания задачи верхней платы: %d\n", ret);
        return;
    }
    
    topTaskRunning = true;
    Serial.println("Задача верхней платы инициализирована (ядро 1, высокий приоритет)");
}

// ============================================
// Запуск задачи нижней платы
// ============================================
void bottomBoardTimerStart() {
    if (!bottomTaskRunning || bottomBoardTaskHandle == nullptr) {
        Serial.println("Задача нижней платы не инициализирована!");
        return;
    }
    
    bottomTaskShouldRun = true;
    Serial.printf("Задача нижней платы запущена (интервал: %d мс)\n", TIMER_BOTTOM_INTERVAL_MS);
}

// ============================================
// Запуск задачи верхней платы
// ============================================
void topBoardTimerStart() {
    if (!topTaskRunning || topBoardTaskHandle == nullptr) {
        Serial.println("Задача верхней платы не инициализирована!");
        return;
    }
    
    topTaskShouldRun = true;
    Serial.printf("Задача верхней платы запущена (интервал: %d мс)\n", TIMER_TOP_INTERVAL_MS);
}

// ============================================
// Остановка задачи нижней платы
// ============================================
void bottomBoardTimerStop() {
    bottomTaskShouldRun = false;
}

// ============================================
// Остановка задачи верхней платы
// ============================================
void topBoardTimerStop() {
    topTaskShouldRun = false;
}

// ============================================
// Общая инициализация обоих задач (для обратной совместимости)
// ============================================
void displayTimerInit() {
    bottomBoardTimerInit();
    topBoardTimerInit();
}

// ============================================
// Общий запуск обоих задач (для обратной совместимости)
// ============================================
void displayTimerStart() {
    bottomBoardTimerStart();
    topBoardTimerStart();
}

// ============================================
// Общая остановка обоих задач (для обратной совместимости)
// ============================================
void displayTimerStop() {
    bottomBoardTimerStop();
    topBoardTimerStop();
}

#else
// ============================================
// Заглушка для платформ, отличных от ESP32
// ============================================
void bottomBoardTimerInit() {
    Serial.println("Задача нижней платы не поддерживается на этой платформе");
}

void topBoardTimerInit() {
    Serial.println("Задача верхней платы не поддерживается на этой платформе");
}

void bottomBoardTimerStart() {
}

void topBoardTimerStart() {
}

void bottomBoardTimerStop() {
}

void topBoardTimerStop() {
}

void displayTimerInit() {
    Serial.println("Задачи мультиплексирования не поддерживаются на этой платформе");
}

void displayTimerStart() {
}

void displayTimerStop() {
}

#endif // ESP32
