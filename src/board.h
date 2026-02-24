#ifndef BOARD_H
#define BOARD_H

#include <Arduino.h>
#include <Wire.h>

// ============================================
// Пины ESP32-S2 Mini для проекта Ruznama Table
// ============================================

// ============================================
// 74HC595 Shift Registers (Верхняя плата)
// ============================================
// Пин LATCH для верхней платы
#define PIN_595_LATCH_TOP     13   // GPIO13 -> Верхняя плата: пин 12 (RCK)

// Пин CLOCK для верхней платы
#define PIN_595_CLOCK_TOP     6    // GPIO6 -> Верхняя плата: пин 11 (SRCK)

// Пин DATA для верхней платы
#define PIN_595_DATA_TOP      8    // GPIO8 -> Верхняя плата: пин 14 (SER)

// ============================================
// 74HC595 Shift Registers (Нижняя плата)
// ============================================
// Пин LATCH для нижней платы
#define PIN_595_LATCH_BOT     12   // GPIO12 -> Нижняя плата: пин 12 (RCK)

// Пин CLOCK для нижней платы
#define PIN_595_CLOCK_BOT     7    // GPIO7 -> Нижняя плата: пин 11 (SRCK)

// Пин DATA для нижней платы
#define PIN_595_DATA_BOT      9    // GPIO9 -> Нижняя плата: пин 14 (SER)

// ============================================
// RTC ZS-042 (DS3231) I2C Interface
// ============================================
// I2C SDA pin для RTC
#define PIN_RTC_SDA       33  // GPIO33 -> ZS-042 (RTC): SDA

// I2C SCL pin для RTC
#define PIN_RTC_SCL       35  // GPIO35 -> ZS-042 (RTC): SCL

// I2C адрес RTC (DS3231)
#define RTC_I2C_ADDR      0x68

// ============================================
// Питание (для справки, не используются в коде)
// ============================================
// GND  -> GND всех плат и БП (Черный провод)
// 3V3  -> VCC Часов ZS-042 (Красный провод - логика)
// 5V   -> VCC всех плат 595 (Красный провод - силовой)

// ============================================
// Макросы для управления 74HC595 (Верхняя плата)
// ============================================
#define LATCH_TOP_LOW()    digitalWrite(PIN_595_LATCH_TOP, LOW)
#define LATCH_TOP_HIGH()   digitalWrite(PIN_595_LATCH_TOP, HIGH)
#define CLOCK_TOP_LOW()    digitalWrite(PIN_595_CLOCK_TOP, LOW)
#define CLOCK_TOP_HIGH()   digitalWrite(PIN_595_CLOCK_TOP, HIGH)
#define DATA_TOP_LOW()     digitalWrite(PIN_595_DATA_TOP, LOW)
#define DATA_TOP_HIGH()    digitalWrite(PIN_595_DATA_TOP, HIGH)

// ============================================
// Макросы для управления 74HC595 (Нижняя плата)
// ============================================
#define LATCH_BOT_LOW()    digitalWrite(PIN_595_LATCH_BOT, LOW)
#define LATCH_BOT_HIGH()   digitalWrite(PIN_595_LATCH_BOT, HIGH)
#define CLOCK_BOT_LOW()    digitalWrite(PIN_595_CLOCK_BOT, LOW)
#define CLOCK_BOT_HIGH()   digitalWrite(PIN_595_CLOCK_BOT, HIGH)
#define DATA_BOT_LOW()     digitalWrite(PIN_595_DATA_BOT, LOW)
#define DATA_BOT_HIGH()    digitalWrite(PIN_595_DATA_BOT, HIGH)

// ============================================
// Инициализация всех пинов
// ============================================
inline void boardInit() {
    // Настройка пинов 74HC595 для верхней платы как выходы
    pinMode(PIN_595_LATCH_TOP, OUTPUT);
    pinMode(PIN_595_CLOCK_TOP, OUTPUT);
    pinMode(PIN_595_DATA_TOP, OUTPUT);
    
    // Настройка пинов 74HC595 для нижней платы как выходы
    pinMode(PIN_595_LATCH_BOT, OUTPUT);
    pinMode(PIN_595_CLOCK_BOT, OUTPUT);
    pinMode(PIN_595_DATA_BOT, OUTPUT);
    
    // Установка начального состояния
    LATCH_TOP_LOW();
    CLOCK_TOP_LOW();
    DATA_TOP_LOW();
    LATCH_BOT_LOW();
    CLOCK_BOT_LOW();
    DATA_BOT_LOW();
    
    // Инициализация I2C для RTC
    Wire.begin(PIN_RTC_SDA, PIN_RTC_SCL);
}

#endif // BOARD_H
