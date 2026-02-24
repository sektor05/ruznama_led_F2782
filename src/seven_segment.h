#ifndef SEVEN_SEGMENT_H
#define SEVEN_SEGMENT_H

#include <Arduino.h>
#include "board.h"
#include "prayer_schedule.h"

// ============================================
// Конфигурация семисегментного индикатора
// ============================================
#define NUM_ROWS           7       // Количество строк (рядов) на рузнама таблице
#define NUM_REGISTERS      5       // Количество регистров 74HC595 в цепочке

// ============================================
// Типы плат
// ============================================
typedef enum {
    BOARD_TOP = 0,    // Верхняя плата
    BOARD_BOTTOM = 1  // Нижняя плата
} BoardType;

// ============================================
// Структура для хранения данных одной строки
// ============================================
struct RowData {
    uint8_t digit1;   // Десятки часов
    uint8_t digit2;   // Единицы часов
    uint8_t digit3;   // Десятки минут
    uint8_t digit4;   // Единицы минут
    bool withColon;   // Двоеточие включено
    bool active;      // Строка активна (для отображения)
};

// ============================================
// Структура для хранения данных всех строк
// ============================================
struct DisplayData {
    RowData rows[7];  // Данные для 7 строк
};

// ============================================
// Коды для семисегментного индикатора
// Формат: a b c d e f g dp (где dp - точка)
// Биты: 0=a, 1=b, 2=c, 3=d, 4=e, 5=f, 6=g, 7=dp
// 1 = сегмент включен, 0 = сегмент выключен
// ============================================
// Общий анод: 0 = сегмент включен, 1 = сегмент выключен
// Общий катод: 1 = сегмент включен, 0 = сегмент выключен
#define SEGMENT_TYPE_COMMON_ANODE   0
#define SEGMENT_TYPE_COMMON_CATHODE 1

// Выберите тип вашего индикатора:
#define SEGMENT_TYPE SEGMENT_TYPE_COMMON_ANODE

// ============================================
// Переворот сегментов на 180 градусов
// Если цифры отображаются вверх ногами, установите это значение в 1
#define FLIP_SEGMENTS_180 1

// ============================================
// Инверсия данных (если используется инвертор на линии данных)
// ============================================
// Если на дисплее ничего не отображается при логической 1 на DATA,
// установите это значение в 1
#define DATA_INVERTED 0

// ============================================
// Порядок отправки битов
// ============================================
// Попробуйте оба варианта, если дисплей не работает корректно
#define BIT_ORDER_LSBFIRST  0
#define BIT_ORDER_MSBFIRST  1

// Выберите порядок отправки битов:
#define BIT_ORDER BIT_ORDER_LSBFIRST

#if SEGMENT_TYPE == SEGMENT_TYPE_COMMON_ANODE
    // Общий анод (0 = включено)
    #define SEG_0   0x88  
    #define SEG_1   0xBB  
    #define SEG_2   0x94  
    #define SEG_3   0x91  
    #define SEG_4   0xA3  
    #define SEG_5   0xC1  
    #define SEG_6   0xC0  
    #define SEG_7   0x9B  
    #define SEG_8   0x80  
    #define SEG_9   0x81  
    #define SEG_TIRE 0xF7  //тире
    #define SEG_OFF 0xFF  // 11111111 (все выключено)
#else
    // Общий катод (1 = включено)
    #define SEG_0   0x3F  // 00111111
    #define SEG_1   0x06  // 00000110
    #define SEG_2   0x5B  // 01011011
    #define SEG_3   0x4F  // 01001111
    #define SEG_4   0x66  // 01100110
    #define SEG_5   0x6D  // 01101101
    #define SEG_6   0x7D  // 01111101
    #define SEG_7   0x07  // 00000111
    #define SEG_8   0x7F  // 01111111
    #define SEG_9   0x6F  // 01101111
    #define SEG_OFF 0x00  // 00000000 (все выключено)
#endif

// ============================================
// Коды для точек (двоеточие между цифрами)
// ============================================
#if SEGMENT_TYPE == SEGMENT_TYPE_COMMON_ANODE
    #define COLON_ON  0x7F  // 01111111 (точка включена на одной цифре)
    #define COLON_OFF 0xFF  // 11111111 (точка выключена)
#else
    #define COLON_ON  0x80  // 10000000 (точка включена на одной цифре)
    #define COLON_OFF 0x00  // 00000000 (точка выключена)
#endif

// ============================================
// Вспомогательные функции
// ============================================

/**
 * @brief Медленная функция shiftOut с задержками
 * 
 * @param dataPin Пин данных
 * @param clockPin Пин тактирования
 * @param bitOrder Порядок битов (LSBFIRST или MSBFIRST)
 * @param val Значение для отправки
 */
void slowShiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val);

// ============================================
// Функции драйвера семисегментного индикатора
// ============================================

/**
 * @brief Инициализация драйвера семисегментного индикатора
 */
void sevenSegmentInit();

/**
 * @brief Отображение данных (времени) на указанном ряду
 * 
 * @param boardType Тип платы (BOARD_TOP или BOARD_BOTTOM)
 * @param hours Часы (0-23)
 * @param minutes Минуты (0-59)
 * @param row Номер ряда (1-6)
 */
void displayData(BoardType boardType, int hours, int minutes, int row);

/**
 * @brief Отображение данных (времени) на указанном ряду (для нижней платы)
 * 
 * @param hours Часы (0-23)
 * @param minutes Минуты (0-59)
 * @param row Номер ряда (1-6)
 */
void displayDataBottom(int hours, int minutes, int row);

/**
 * @brief Очистка всех индикаторов на указанной плате
 * 
 * @param boardType Тип платы (BOARD_TOP или BOARD_BOTTOM)
 */
void clearDisplay(BoardType boardType);

/**
 * @brief Очистка всех индикаторов на нижней плате
 */
void clearDisplayBottom();

/**
 * @brief Отправка массива данных в регистры 74HC595
 * 
 * @param data Массив из 5 байт данных
 * @param boardType Тип платы (BOARD_TOP или BOARD_BOTTOM)
 */
void shiftOutData(uint8_t data[NUM_REGISTERS], BoardType boardType);

// ============================================
// Функции для динамической индикации намазов
// ============================================

/**
 * @brief Инициализация данных отображения
 */
void initDisplayData();

/**
 * @brief Установка данных для строки
 * 
 * @param row Номер строки (1-7)
 * @param hours Часы
 * @param minutes Минуты
 * @param withColon Включить двоеточие
 */
void setRowData(int row, int hours, int minutes, bool withColon);

/**
 * @brief Очистка данных строки
 * 
 * @param row Номер строки (1-7)
 */
void clearRowData(int row);

/**
 * @brief Мультиплексированное отображение всех строк
 * 
 * @param boardType Тип платы (BOARD_TOP или BOARD_BOTTOM)
 */
void multiplexDisplay(BoardType boardType);

/**
 * @brief Отображение всех молитв на соответствующих строках (1-6)
 * 
 * @param boardType Тип платы (BOARD_TOP или BOARD_BOTTOM)
 * @param schedule Расписание молитв на день
 */
void displayAllPrayers(BoardType boardType, const DailySchedule& schedule);

/**
 * @brief Отображение текущего времени и минут до икамата на 7-й строке
 * 
 * @param boardType Тип платы (BOARD_TOP или BOARD_BOTTOM)
 * @param hours Часы текущего времени
 * @param minutes Минуты текущего времени
 * @param minutesToIqamah Минуты до икамата (-1 если нет активного икамата)
 */
void displayTimeWithIqamah(BoardType boardType, int hours, int minutes, int minutesToIqamah);

/**
 * @brief Отображение только текущего времени на 7-й строке
 * 
 * @param boardType Тип платы (BOARD_TOP или BOARD_BOTTOM)
 * @param hours Часы текущего времени
 * @param minutes Минуты текущего времени
 */
void displayCurrentTime(BoardType boardType, int hours, int minutes);

/**
 * @brief Отображение минут до икамата на 7-й строке
 * 
 * @param boardType Тип платы (BOARD_TOP или BOARD_BOTTOM)
 * @param minutesToIqamah Минуты до икамата
 */
void displayMinutesToIqamah(BoardType boardType, int minutesToIqamah);

/**
 * @brief Очистка 7-й строки (отключение индикации)
 * 
 * @param boardType Тип платы (BOARD_TOP или BOARD_BOTTOM)
 */
void clearRow7(BoardType boardType);

/**
 * @brief Отображение времени до икамата на 7-й строке с миганием
 * 
 * @param boardType Тип платы (BOARD_TOP или BOARD_BOTTOM)
 * @param timeToIqamah Время до икамата (минуты или секунды)
 * @param blinkState Состояние мигания (true = включено, false = выключено)
 */
void displayToIqamah(BoardType boardType, int timeToIqamah, bool blinkState);

/**
 * @brief Отображение времени молитвы на указанной строке с миганием
 * 
 * @param boardType Тип платы (BOARD_TOP или BOARD_BOTTOM)
 * @param row Номер строки (1-6)
 * @param hours Часы
 * @param minutes Минуты
 * @param blinkState Состояние мигания (true = включено, false = выключено)
 */
void displayPrayerTimeBlink(BoardType boardType, int row, int hours, int minutes, bool blinkState);

#endif // SEVEN_SEGMENT_H
