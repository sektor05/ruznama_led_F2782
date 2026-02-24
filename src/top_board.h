#ifndef TOP_BOARD_H
#define TOP_BOARD_H

#include <Arduino.h>
#include "board.h"
#include "seven_segment.h"

// ============================================
// Конфигурация верхней платы
// ============================================
#define TOP_NUM_REGISTERS  5       // Количество регистров 74HC595 на верхней плате
#define TOP_DATA_PACKET_SIZE 5     // Размер пакета данных (5 байт)

// ============================================
// Индексы байтов в пакете данных
// ============================================
typedef enum {
    TOP_BYTE_CONTROL = 0,      // Байт 0: управление динамической индикацией
    TOP_BYTE_TIME = 1,         // Байт 1: семисегментные индикаторы времени ЧЧ:ММ:СС
    TOP_BYTE_DATE = 2,        // Байт 2: семисегментные индикаторы даты ГГГГ.ММ.ДД
    TOP_BYTE_WEEKDAY = 3,     // Байт 3: день недели и индикатор григорианского календаря
    TOP_BYTE_HIJRI = 4        // Байт 4: месяц Хиджры и индикатор хиджра календаря
} TopByteIndex;

// ============================================
// Биты байта управления (TOP_BYTE_CONTROL) - для времени
// ============================================
#define TOP_COLON_BIT      7    // Бит 7: двоеточие между секундами и минутами
#define TOP_YEAR_THOUSAND_BIT 0   // Бит 0: первая цифра года (тысячная года)
#define TOP_HOURS_TENS_BIT 1     // Бит 1: часы десятки
#define TOP_HOURS_UNITS_BIT 2    // Бит 2: часы единицы
#define TOP_MINUTES_TENS_BIT 3   // Бит 3: минуты десятки
#define TOP_MINUTES_UNITS_BIT 4   // Бит 4: минуты единицы
#define TOP_SECONDS_TENS_BIT 5   // Бит 5: секунды десятки
#define TOP_SECONDS_UNITS_BIT 6   // Бит 6: секунды единицы

// ============================================
// Биты байта даты (TOP_BYTE_DATE) - для даты
// ============================================
#define TOP_DATE_YEAR_HUNDREDS_BIT 0   // Бит 0: сотая года
#define TOP_DATE_YEAR_TENS_BIT 1       // Бит 1: десятки года
#define TOP_DATE_YEAR_UNITS_BIT 2      // Бит 2: единицы года
#define TOP_DATE_MONTH_TENS_BIT 3      // Бит 3: десятки месяца
#define TOP_DATE_MONTH_UNITS_BIT 4     // Бит 4: единицы месяца
#define TOP_DATE_DAY_TENS_BIT 5        // Бит 5: десятки даты
#define TOP_DATE_DAY_UNITS_BIT 6       // Бит 6: единицы даты
#define TOP_DATE_COLON_BIT 7           // Бит 7: двоеточие (для даты выключено)

// ============================================
// Биты байта дня недели (TOP_BYTE_WEEKDAY)
// ============================================
#define TOP_GREGORIAN_LED_BIT 0  // Бит 0: светодиод григорианского календаря

// ============================================
// Биты байта Хиджры (TOP_BYTE_HIJRI)
// ============================================
#define TOP_HIJRI_LED_BIT 0      // Бит 0: светодиод хиджра календаря

// ============================================
// Значения для дней недели (светодиоды)
// ============================================
typedef enum {
    WEEKDAY_SATURDAY    = 0x90,  // Суббота
    WEEKDAY_SUNDAY      = 0xCC,  // Воскресенье
    WEEKDAY_MONDAY      = 0xCA,  // Понедельник
    WEEKDAY_TUESDAY     = 0x8E,  // Вторник
    WEEKDAY_WEDNESDAY   = 0x4E,  // Среда
    WEEKDAY_THURSDAY    = 0x56,  // Четверг
    WEEKDAY_FRIDAY      = 0x66   // Пятница
} WeekdayCode;

// ============================================
// Значения для месяцев Хиджры (светодиоды)
// ============================================
typedef enum {
    HIJRI_MUHARRAM      = 0xE4,  // Мухаррам
    HIJRI_SAFAR         = 0xE2,  // Сафар
    HIJRI_RABI_AL_AWWAL = 0xA6,  // Раби аль-авваль
    HIJRI_RABI_AS_SANI  = 0x96,  // Раби ас-сани
    HIJRI_JUMADA_AL_AWWAL = 0xD2, // Джумада аль-уля
    HIJRI_JUMADA_AS_SANI = 0xD4, // Джумада аль-ахира
    HIJRI_RAJAB         = 0xCC,  // Раджаб
    HIJRI_SHAABAN       = 0xCA,  // Шаабан
    HIJRI_RAMADAN       = 0x8E,  // Рамадан
    HIJRI_SHAWWAL       = 0x4E,  // Шавваль
    HIJRI_DHU_AL_QADA   = 0x56,  // Зу-ль-када
    HIJRI_DHU_AL_HIJJAH = 0x66   // Зу-ль-хиджа
} HijriMonthCode;

// ============================================
// Типы календаря
// ============================================
typedef enum {
    CALENDAR_GREGORIAN = 0,  // Григорианский календарь
    CALENDAR_HIJRI     = 1   // Хиджра календарь
} CalendarType;

// ============================================
// Режимы отображения для динамической индикации
// ============================================
typedef enum {
    DISPLAY_MODE_TIME = 0,    // Отображение времени
    DISPLAY_MODE_DATE = 1     // Отображение даты
} DisplayMode;

// ============================================
// Структура для хранения данных верхней платы
// ============================================
struct TopBoardData {
    uint16_t year;          // Год (например, 2026)
    uint8_t month;          // Месяц (1-12)
    uint8_t day;            // День (1-31)
    uint8_t hours;          // Часы (0-23)
    uint8_t minutes;        // Минуты (0-59)
    uint8_t seconds;        // Секунды (0-59)
    WeekdayCode weekday;    // День недели
    HijriMonthCode hijriMonth; // Месяц Хиджры
    CalendarType calendarType; // Тип календаря
    bool colonEnabled;     // Двоеточие включено
};

// ============================================
// Функции библиотеки верхней платы
// ============================================

/**
 * @brief Инициализация библиотеки верхней платы
 */
void topBoardInit();

/**
 * @brief Отправка пакета данных из 5 байт на верхнюю плату
 * 
 * @param data Массив из 5 байт данных
 */
void sendTopBoardData(uint8_t data[TOP_DATA_PACKET_SIZE]);

/**
 * @brief Очистка всех индикаторов на верхней плате
 */
void clearTopBoard();

/**
 * @brief Отображение даты и времени на верхней плате
 * 
 * @param year Год (например, 2026)
 * @param month Месяц (1-12)
 * @param day День (1-31)
 * @param hours Часы (0-23)
 * @param minutes Минуты (0-59)
 * @param seconds Секунды (0-59)
 * @param weekday День недели
 * @param hijriMonth Месяц Хиджры
 * @param calendarType Тип календаря (CALENDAR_GREGORIAN или CALENDAR_HIJRI)
 * @param colonEnabled Включить двоеточие между секундами и минутами
 */
void displayDateTime(uint16_t year, uint8_t month, uint8_t day,
                    uint8_t hours, uint8_t minutes, uint8_t seconds,
                    WeekdayCode weekday, HijriMonthCode hijriMonth,
                    CalendarType calendarType, bool colonEnabled);

/**
 * @brief Отображение даты и времени из структуры TopBoardData
 * 
 * @param data Структура с данными для отображения
 */
void displayDateTimeFromData(const TopBoardData& data);

/**
 * @brief Формирование пакета данных для отправки
 * 
 * @param data Структура с данными для отображения
 * @param output Массив из 5 байт для заполнения
 */
void buildDataPacket(const TopBoardData& data, uint8_t output[TOP_DATA_PACKET_SIZE]);

/**
 * @brief Получение кода дня недели по номеру дня (0-6, где 0 = Суббота)
 * 
 * @param dayNumber Номер дня недели (0-6)
 * @return WeekdayCode Код дня недели
 */
WeekdayCode getWeekdayCode(uint8_t dayNumber);

/**
 * @brief Получение кода месяца Хиджры по номеру месяца (1-12)
 * 
 * @param monthNumber Номер месяца Хиджры (1-12)
 * @return HijriMonthCode Код месяца Хиджры
 */
HijriMonthCode getHijriMonthCode(uint8_t monthNumber);

/**
 * @brief Получение названия дня недели
 * 
 * @param weekday Код дня недели
 * @return const char* Название дня недели
 */
const char* getWeekdayName(WeekdayCode weekday);

/**
 * @brief Получение названия месяца Хиджры
 * 
 * @param month Код месяца Хиджры
 * @return const char* Название месяца Хиджры
 */
const char* getHijriMonthName(HijriMonthCode month);

// ============================================
// Функции для динамической индикации
// ============================================

/**
 * @brief Инициализация данных для динамической индикации
 */
void initTopBoardData();

/**
 * @brief Установка данных для отображения
 * 
 * @param data Структура с данными для отображения
 */
void setTopBoardData(const TopBoardData& data);

/**
 * @brief Динамическая индикация - переключение между временем и датой
 * Вызывать в loop() для переключения каждые 500 мс
 */
void multiplexTopBoard();

/**
 * @brief Отображение времени на верхней плате
 * 
 * @param hours Часы (0-23)
 * @param minutes Минуты (0-59)
 * @param seconds Секунды (0-59)
 * @param colonEnabled Включить двоеточие
 */
void displayTime(uint8_t hours, uint8_t minutes, uint8_t seconds, bool colonEnabled);

/**
 * @brief Отображение даты на верхней плате
 * 
 * @param year Год (например, 2026)
 * @param month Месяц (1-12)
 * @param day День (1-31)
 */
void displayDate(uint16_t year, uint8_t month, uint8_t day);

#endif // TOP_BOARD_H
