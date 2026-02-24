# Библиотека верхней платы (Top Board Library)

## Обзор

Библиотека [`top_board.h`](src/top_board.h) и [`top_board.cpp`](src/top_board.cpp) предназначена для управления верхней платой дисплея, которая отображает дату, время, день недели и месяц по Хиджре.

## Функциональность

### Отображаемые данные

- **Дата**: формат ГГГГ.ММ.ДД (например, 2026.02.19)
- **Время**: формат ЧЧ:ММ:СС (например, 13:22:40)
- **День недели**: отображается светодиодами
- **Месяц Хиджры**: отображается светодиодами
- **Тип календаря**: индикация Григорианского/Хиджра календаря двумя отдельными светодиодами

## Протокол данных

Библиотека принимает пакет данных из 5 байт:

### Байт 0: Управление динамической индикацией

| Бит | Описание |
|-----|----------|
| 7   | Включает двоеточие между секундами и минутами (1 = включено, 0 = выключено) |
| 0   | Первая цифра года (0 = '1', 1 = '2') |
| 1-7 | Данные часов, минут и секунд (BCD код) |

### Байт 1: Семисегментные индикаторы времени ЧЧ:ММ:СС

Реализует логику как для нижней платы с использованием условия `#if SEGMENT_TYPE == SEGMENT_TYPE_COMMON_ANODE`.

### Байт 2: Семисегментные индикаторы даты ГГГГ.ММ.ДД

Содержит коды для отображения даты.

### Байт 3: День недели и индикатор григорианского календаря

| Значение | День недели |
|----------|-------------|
| 0x90     | Суббота |
| 0xCC     | Воскресенье |
| 0xCA     | Понедельник |
| 0x8E     | Вторник |
| 0x4E     | Среда |
| 0x56     | Четверг |
| 0x66     | Пятница |

**Бит 0**: Включает светодиод григорианского календаря (1 = включено, 0 = выключено)

### Байт 4: Месяцы Хиджры и индикатор хиджра календаря

| Значение | Месяц Хиджры |
|----------|--------------|
| 0xE4     | Мухаррам |
| 0xE2     | Сафар |
| 0xA6     | Раби аль-авваль |
| 0x96     | Раби ас-сани |
| 0xD2     | Джумада аль-уля |
| 0xD4     | Джумада аль-ахира |
| 0xCC     | Раджаб |
| 0xCA     | Шаабан |
| 0x8E     | Рамадан |
| 0x4E     | Шавваль |
| 0x56     | Зу-ль-када |
| 0x66     | Зу-ль-хиджа |

**Бит 0**: Включает светодиод хиджра календаря (1 = включено, 0 = выключено)

## Использование

### Инициализация

```cpp
#include "top_board.h"

void setup() {
    Serial.begin(115200);
    boardInit();      // Инициализация пинов (из board.h)
    topBoardInit();   // Инициализация библиотеки верхней платы
}
```

### Отображение даты и времени

```cpp
// Отображение даты и времени
displayDateTime(
    2026,              // год
    2,                 // месяц
    19,                // день
    13,                // часы
    22,                // минуты
    40,                // секунды
    WEEKDAY_THURSDAY,  // день недели
    HIJRI_RAJAB,       // месяц Хиджры
    CALENDAR_GREGORIAN, // тип календаря
    true               // двоеточие включено
);
```

### Использование структуры TopBoardData

```cpp
TopBoardData data;
data.year = 2026;
data.month = 2;
data.day = 19;
data.hours = 13;
data.minutes = 22;
data.seconds = 40;
data.weekday = WEEKDAY_THURSDAY;
data.hijriMonth = HIJRI_RAJAB;
data.calendarType = CALENDAR_GREGORIAN;
data.colonEnabled = true;

displayDateTimeFromData(data);
```

### Получение кодов дней недели и месяцев Хиджры

```cpp
// Получение кода дня недели по номеру (0-6, где 0 = Суббота)
WeekdayCode weekday = getWeekdayCode(3); // Вторник

// Получение названия дня недели
const char* name = getWeekdayName(WEEKDAY_THURSDAY); // "Четверг"

// Получение кода месяца Хиджры по номеру (1-12)
HijriMonthCode month = getHijriMonthCode(7); // Раджаб

// Получение названия месяца Хиджры
const char* hijriName = getHijriMonthName(HIJRI_RAJAB); // "Раджаб"
```

### Очистка дисплея

```cpp
clearTopBoard();
```

## API

### Функции

| Функция | Описание |
|---------|----------|
| `topBoardInit()` | Инициализация библиотеки верхней платы |
| `sendTopBoardData(uint8_t data[5])` | Отправка пакета данных из 5 байт |
| `clearTopBoard()` | Очистка всех индикаторов |
| `displayDateTime(...)` | Отображение даты и времени |
| `displayDateTimeFromData(const TopBoardData&)` | Отображение из структуры |
| `buildDataPacket(const TopBoardData&, uint8_t[5])` | Формирование пакета данных |
| `getWeekdayCode(uint8_t)` | Получение кода дня недели |
| `getHijriMonthCode(uint8_t)` | Получение кода месяца Хиджры |
| `getWeekdayName(WeekdayCode)` | Получение названия дня недели |
| `getHijriMonthName(HijriMonthCode)` | Получение названия месяца Хиджры |

### Структуры

#### TopBoardData

```cpp
struct TopBoardData {
    uint8_t year;           // Год (например, 2026)
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
```

### Перечисления

#### WeekdayCode

```cpp
typedef enum {
    WEEKDAY_SATURDAY    = 0x90,  // Суббота
    WEEKDAY_SUNDAY      = 0xCC,  // Воскресенье
    WEEKDAY_MONDAY      = 0xCA,  // Понедельник
    WEEKDAY_TUESDAY     = 0x8E,  // Вторник
    WEEKDAY_WEDNESDAY   = 0x4E,  // Среда
    WEEKDAY_THURSDAY    = 0x56,  // Четверг
    WEEKDAY_FRIDAY      = 0x66   // Пятница
} WeekdayCode;
```

#### HijriMonthCode

```cpp
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
```

#### CalendarType

```cpp
typedef enum {
    CALENDAR_GREGORIAN = 0,  // Григорианский календарь
    CALENDAR_HIJRI     = 1   // Хиджра календарь
} CalendarType;
```

## Тестирование

Для тестирования библиотеки используйте основной проект. Пример использования:

```cpp
#include "top_board.h"

void setup() {
    Serial.begin(115200);
    boardInit();
    topBoardInit();
    
    // Отображаем текущую дату и время
    displayDateTime(
        2026,              // год
        2,                 // месяц
        19,                // день
        13,                // часы
        22,                // минуты
        40,                // секунды
        WEEKDAY_THURSDAY,  // день недели
        HIJRI_RAJAB,       // месяц Хиджры
        CALENDAR_GREGORIAN, // тип календаря
        true               // двоеточие включено
    );
}

void loop() {
    // Ваш код
    delay(1000);
}
```

## Конфигурация

### Пины верхней платы

Определены в [`board.h`](src/board.h):

```cpp
#define PIN_595_LATCH_TOP     13   // GPIO13 -> Верхняя плата: пин 12 (RCK)
#define PIN_595_CLOCK_TOP     6    // GPIO6 -> Верхняя плата: пин 11 (SRCK)
#define PIN_595_DATA_TOP      8    // GPIO8 -> Верхняя плата: пин 14 (SER)
```

### Макросы управления

```cpp
#define LATCH_TOP_LOW()    digitalWrite(PIN_595_LATCH_TOP, LOW)
#define LATCH_TOP_HIGH()   digitalWrite(PIN_595_LATCH_TOP, HIGH)
#define CLOCK_TOP_LOW()    digitalWrite(PIN_595_CLOCK_TOP, LOW)
#define CLOCK_TOP_HIGH()   digitalWrite(PIN_595_CLOCK_TOP, HIGH)
#define DATA_TOP_LOW()     digitalWrite(PIN_595_DATA_TOP, LOW)
#define DATA_TOP_HIGH()    digitalWrite(PIN_595_DATA_TOP, HIGH)
```

## Зависимости

- [`board.h`](src/board.h) - определения пинов и макросы
- [`seven_segment.h`](src/seven_segment.h) - функции для работы с семисегментными индикаторами

## Примечания

1. Библиотека использует те же коды для семисегментных индикаторов, что и [`seven_segment.cpp`](src/seven_segment.cpp)
2. Для корректной работы необходимо, чтобы `SEGMENT_TYPE` был определен как `SEGMENT_TYPE_COMMON_ANODE` или `SEGMENT_TYPE_COMMON_CATHODE`
3. Библиотека поддерживает инверсию данных через макрос `DATA_INVERTED`
4. Порядок отправки битов определяется макросом `BIT_ORDER`

## Пример полного использования

```cpp
#include <Arduino.h>
#include "top_board.h"

void setup() {
    Serial.begin(115200);
    boardInit();
    topBoardInit();
    
    // Отображаем текущую дату и время
    displayDateTime(
        2026,              // год
        2,                 // месяц
        19,                // день
        13,                // часы
        22,                // минуты
        40,                // секунды
        WEEKDAY_THURSDAY,  // день недели
        HIJRI_RAJAB,       // месяц Хиджры
        CALENDAR_GREGORIAN, // тип календаря
        true               // двоеточие включено
    );
}

void loop() {
    // Ваш код
    delay(1000);
}
```

## Лицензия

Данная библиотека является частью проекта Ruznama Table и распространяется под той же лицензией.
