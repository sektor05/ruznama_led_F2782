#include "top_board.h"

// ============================================
// Глобальные данные для динамической индикации
// ============================================
static TopBoardData g_topBoardData;
static DisplayMode g_displayMode = DISPLAY_MODE_TIME;
static unsigned long g_lastSwitchTime = 0;
static const unsigned long SWITCH_INTERVAL = 3; // Переключение каждые 500 мс

// ============================================
// Коды цифр для семисегментного индикатора (общий анод)
// Используем те же коды, что и в seven_segment.h
// ============================================
#if SEGMENT_TYPE == SEGMENT_TYPE_COMMON_ANODE
static const uint8_t digitCodes[10] = {
    0x88,  // 0
    0xBB,  // 1
    0x94,  // 2
    0x91,  // 3
    0xA3,  // 4
    0xC1,  // 5
    0xC0,  // 6
    0x9B,  // 7
    0x80,  // 8
    0x81   // 9
};
#else
static const uint8_t digitCodes[10] = {
    0x3F,  // 0
    0x06,  // 1
    0x5B,  // 2
    0x4F,  // 3
    0x66,  // 4
    0x6D,  // 5
    0x7D,  // 6
    0x07,  // 7
    0x7F,  // 8
    0x6F   // 9
};
#endif

// ============================================
// Коды для выключенного сегмента
// ============================================
#if SEGMENT_TYPE == SEGMENT_TYPE_COMMON_ANODE
#define SEG_OFF 0xFF
#else
#define SEG_OFF 0x00
#endif

// ============================================
// Названия дней недели
// ============================================
static const char* weekdayNames[] = {
    "Суббота",      // 0
    "Воскресенье",  // 1
    "Понедельник",  // 2
    "Вторник",      // 3
    "Среда",        // 4
    "Четверг",      // 5
    "Пятница"       // 6
};

// ============================================
// Названия месяцев Хиджры
// ============================================
static const char* hijriMonthNames[] = {
    "Мухаррам",      // 0
    "Сафар",         // 1
    "Раби аль-авваль", // 2
    "Раби ас-сани",  // 3
    "Джумада аль-уля", // 4
    "Джумада аль-ахира", // 5
    "Раджаб",        // 6
    "Шаабан",        // 7
    "Рамадан",       // 8
    "Шавваль",       // 9
    "Зу-ль-када",    // 10
    "Зу-ль-хиджа"    // 11
};

// ============================================
// Инициализация библиотеки верхней платы
// ============================================
void topBoardInit() {
    // Пины уже инициализированы в boardInit()
    // Эта функция оставлена для совместимости и возможного расширения
    clearTopBoard();
}

// ============================================
// Отправка пакета данных из 5 байт на верхнюю плату
// ============================================
void sendTopBoardData(uint8_t data[TOP_DATA_PACKET_SIZE]) {
    // Опускаем LATCH для подготовки к приему данных
    LATCH_TOP_LOW();
    delayMicroseconds(1);
    
    // Отправляем байты в порядке: [0] -> [4]
    for (int i = 0; i < TOP_DATA_PACKET_SIZE; i++) {
        uint8_t byteToSend = data[i];
#if DATA_INVERTED
        byteToSend = ~byteToSend;
#endif
#if BIT_ORDER == BIT_ORDER_LSBFIRST
        slowShiftOut(PIN_595_DATA_TOP, PIN_595_CLOCK_TOP, LSBFIRST, byteToSend);
#else
        slowShiftOut(PIN_595_DATA_TOP, PIN_595_CLOCK_TOP, MSBFIRST, byteToSend);
#endif
    }
    
    // Поднимаем LATCH для фиксации данных
    LATCH_TOP_HIGH();
    delayMicroseconds(1);
}

// ============================================
// Очистка всех индикаторов на верхней плате
// ============================================
void clearTopBoard() {
    uint8_t data[TOP_DATA_PACKET_SIZE];
    for (int i = 0; i < TOP_DATA_PACKET_SIZE; i++) {
        data[i] = 0x00;
    }
    sendTopBoardData(data);
}

// ============================================
// Формирование пакета данных для отправки
// ============================================
void buildDataPacket(const TopBoardData& data, uint8_t output[TOP_DATA_PACKET_SIZE]) {
    // ============================================
    // Байт 0: управление динамической индикацией
    // ============================================
    output[TOP_BYTE_CONTROL] = 0x00;
    
    // Бит 7: двоеточие между секундами и минутами
    if (data.colonEnabled) {
        output[TOP_BYTE_CONTROL] |= (1 << TOP_COLON_BIT);
    }
    
    // Бит 0: первая цифра года (0 = '1', 1 = '2')
    // Определяем первую цифру года (для 2026 это '2')
    uint8_t firstYearDigit = (data.year / 1000) % 10;
    if (firstYearDigit == 2) {
        output[TOP_BYTE_CONTROL] |= (1 << TOP_YEAR_THOUSAND_BIT);
    }
    
    // Биты 1-7: данные часов, минут и секунд
    // Формируем BCD код для времени ЧЧ:ММ:СС
    // Часы (0-23) -> биты 1-4 (десятки часов в битах 1-2, единицы часов в битах 3-4)
    // Минуты (0-59) -> биты 5-6 (десятки минут)
    // Секунды (0-59) -> биты 7 (десятки секунд)
    
    // Десятки часов (0-2) -> биты 1-2
    uint8_t hoursTens = data.hours / 10;
    output[TOP_BYTE_CONTROL] |= ((hoursTens & 0x03) << 1);
    
    // Единицы часов (0-9) -> биты 3-4
    uint8_t hoursUnits = data.hours % 10;
    output[TOP_BYTE_CONTROL] |= ((hoursUnits & 0x0F) << 3);
    
    // Десятки минут (0-5) -> биты 5-6
    uint8_t minutesTens = data.minutes / 10;
    output[TOP_BYTE_CONTROL] |= ((minutesTens & 0x07) << 5);
    
    // Единицы минут (0-9) -> сохраняем в отдельной переменной для использования в байте 1
    
    // Десятки секунд (0-5) -> биты 7 (но бит 7 уже занят двоеточием, поэтому используем другой подход)
    // Примечание: байт 0 имеет ограниченное количество битов, поэтому часть данных времени
    // может быть закодирована иначе. Здесь используем упрощенный подход.
    
    // ============================================
    // Байт 1: семисегментные индикаторы времени ЧЧ:ММ:СС
    // ============================================
    // Формируем коды для отображения времени ЧЧ:ММ:СС
    // Используем логику как для нижней платы с SEGMENT_TYPE_COMMON_ANODE
    
    // Получаем коды для цифр
    uint8_t h1 = digitCodes[data.hours / 10];        // Десятки часов
    uint8_t h2 = digitCodes[data.hours % 10];        // Единицы часов
    uint8_t m1 = digitCodes[data.minutes / 10];      // Десятки минут
    uint8_t m2 = digitCodes[data.minutes % 10];      // Единицы минут
    uint8_t s1 = digitCodes[data.seconds / 10];      // Десятки секунд
    uint8_t s2 = digitCodes[data.seconds % 10];      // Единицы секунд
    
    // Формируем байт 1 для отображения времени
    // В зависимости от конфигурации платы, байт 1 может содержать коды для разных цифр
    // Здесь используем упрощенный подход: байт 1 содержит код для первой цифры времени
    output[TOP_BYTE_TIME] = h1;
    
    // Примечание: для полного отображения времени ЧЧ:ММ:СС может потребоваться
    // отправка нескольких байтов или использование динамической индикации
    // В данном случае мы используем байт 1 для первой цифры времени
    
    // ============================================
    // Байт 2: семисегментные индикаторы даты ГГГГ.ММ.ДД
    // ============================================
    // Формируем коды для отображения даты ГГГГ.ММ.ДД
    uint8_t y1 = digitCodes[(data.year / 1000) % 10];  // Первая цифра года
    uint8_t y2 = digitCodes[(data.year / 100) % 10];   // Вторая цифра года
    uint8_t y3 = digitCodes[(data.year / 10) % 10];    // Третья цифра года
    uint8_t y4 = digitCodes[data.year % 10];            // Четвертая цифра года
    uint8_t mon1 = digitCodes[data.month / 10];        // Десятки месяца
    uint8_t mon2 = digitCodes[data.month % 10];        // Единицы месяца
    uint8_t d1 = digitCodes[data.day / 10];            // Десятки дня
    uint8_t d2 = digitCodes[data.day % 10];            // Единицы дня
    
    // Формируем байт 2 для отображения даты
    // Здесь используем упрощенный подход: байт 2 содержит код для первой цифры даты
    output[TOP_BYTE_DATE] = y1;
    
    // ============================================
    // Байт 3: день недели и индикатор григорианского календаря
    // ============================================
    output[TOP_BYTE_WEEKDAY] = data.weekday;
    
    // Бит 0: светодиод григорианского календаря
    if (data.calendarType == CALENDAR_GREGORIAN) {
        output[TOP_BYTE_WEEKDAY] |= (1 << TOP_GREGORIAN_LED_BIT);
    }
    
    // ============================================
    // Байт 4: месяц Хиджры и индикатор хиджра календаря
    // ============================================
    output[TOP_BYTE_HIJRI] = data.hijriMonth;
    
    // Бит 0: светодиод хиджра календаря
    if (data.calendarType == CALENDAR_HIJRI) {
        output[TOP_BYTE_HIJRI] |= (1 << TOP_HIJRI_LED_BIT);
    }
}

// ============================================
// Отображение даты и времени на верхней плате
// ============================================
void displayDateTime(uint16_t year, uint8_t month, uint8_t day,
                    uint8_t hours, uint8_t minutes, uint8_t seconds,
                    WeekdayCode weekday, HijriMonthCode hijriMonth,
                    CalendarType calendarType, bool colonEnabled) {
    TopBoardData data;
    data.year = year;
    data.month = month;
    data.day = day;
    data.hours = hours;
    data.minutes = minutes;
    data.seconds = seconds;
    data.weekday = weekday;
    data.hijriMonth = hijriMonth;
    data.calendarType = calendarType;
    data.colonEnabled = colonEnabled;
    
    displayDateTimeFromData(data);
}

// ============================================
// Отображение даты и времени из структуры TopBoardData
// ============================================
void displayDateTimeFromData(const TopBoardData& data) {
    uint8_t packet[TOP_DATA_PACKET_SIZE];
    buildDataPacket(data, packet);
    sendTopBoardData(packet);
}

// ============================================
// Получение кода дня недели по номеру дня (0-6, где 0 = Суббота)
// ============================================
WeekdayCode getWeekdayCode(uint8_t dayNumber) {
    switch (dayNumber) {
        case 0: return WEEKDAY_SATURDAY;
        case 1: return WEEKDAY_SUNDAY;
        case 2: return WEEKDAY_MONDAY;
        case 3: return WEEKDAY_TUESDAY;
        case 4: return WEEKDAY_WEDNESDAY;
        case 5: return WEEKDAY_THURSDAY;
        case 6: return WEEKDAY_FRIDAY;
        default: return WEEKDAY_SATURDAY;
    }
}

// ============================================
// Получение кода месяца Хиджры по номеру месяца (1-12)
// ============================================
HijriMonthCode getHijriMonthCode(uint8_t monthNumber) {
    if (monthNumber < 1 || monthNumber > 12) {
        monthNumber = 1;
    }
    
    switch (monthNumber) {
        case 1:  return HIJRI_MUHARRAM;
        case 2:  return HIJRI_SAFAR;
        case 3:  return HIJRI_RABI_AL_AWWAL;
        case 4:  return HIJRI_RABI_AS_SANI;
        case 5:  return HIJRI_JUMADA_AL_AWWAL;
        case 6:  return HIJRI_JUMADA_AS_SANI;
        case 7:  return HIJRI_RAJAB;
        case 8:  return HIJRI_SHAABAN;
        case 9:  return HIJRI_RAMADAN;
        case 10: return HIJRI_SHAWWAL;
        case 11: return HIJRI_DHU_AL_QADA;
        case 12: return HIJRI_DHU_AL_HIJJAH;
        default: return HIJRI_MUHARRAM;
    }
}

// ============================================
// Получение названия дня недели
// ============================================
const char* getWeekdayName(WeekdayCode weekday) {
    switch (weekday) {
        case WEEKDAY_SATURDAY:    return weekdayNames[0];
        case WEEKDAY_SUNDAY:      return weekdayNames[1];
        case WEEKDAY_MONDAY:      return weekdayNames[2];
        case WEEKDAY_TUESDAY:     return weekdayNames[3];
        case WEEKDAY_WEDNESDAY:   return weekdayNames[4];
        case WEEKDAY_THURSDAY:    return weekdayNames[5];
        case WEEKDAY_FRIDAY:      return weekdayNames[6];
        default:                  return weekdayNames[0];
    }
}

// ============================================
// Получение названия месяца Хиджры
// ============================================
const char* getHijriMonthName(HijriMonthCode month) {
    switch (month) {
        case HIJRI_MUHARRAM:      return hijriMonthNames[0];
        case HIJRI_SAFAR:         return hijriMonthNames[1];
        case HIJRI_RABI_AL_AWWAL: return hijriMonthNames[2];
        case HIJRI_RABI_AS_SANI:  return hijriMonthNames[3];
        case HIJRI_JUMADA_AL_AWWAL: return hijriMonthNames[4];
        case HIJRI_JUMADA_AS_SANI: return hijriMonthNames[5];
        case HIJRI_RAJAB:         return hijriMonthNames[6];
        case HIJRI_SHAABAN:       return hijriMonthNames[7];
        case HIJRI_RAMADAN:       return hijriMonthNames[8];
        case HIJRI_SHAWWAL:       return hijriMonthNames[9];
        case HIJRI_DHU_AL_QADA:   return hijriMonthNames[10];
        case HIJRI_DHU_AL_HIJJAH: return hijriMonthNames[11];
        default:                  return hijriMonthNames[0];
    }
}

// ============================================
// Функции для динамической индикации
// ============================================

/**
 * @brief Инициализация данных для динамической индикации
 */
void initTopBoardData() {
    // Инициализируем данные нулями
    g_topBoardData.year = 26;
    g_topBoardData.month = 1;
    g_topBoardData.day = 1;
    g_topBoardData.hours = 0;
    g_topBoardData.minutes = 0;
    g_topBoardData.seconds = 0;
    g_topBoardData.weekday = WEEKDAY_SATURDAY;
    g_topBoardData.hijriMonth = HIJRI_MUHARRAM;
    g_topBoardData.calendarType = CALENDAR_GREGORIAN;
    g_topBoardData.colonEnabled = true;
    
    g_displayMode = DISPLAY_MODE_TIME;
    g_lastSwitchTime = millis();
}

/**
 * @brief Установка данных для отображения
 */
void setTopBoardData(const TopBoardData& data) {
    g_topBoardData = data;
}

/**
 * @brief Отображение времени на верхней плате
 */
void displayTime(uint8_t hours, uint8_t minutes, uint8_t seconds, bool colonEnabled) {
    uint8_t packet[TOP_DATA_PACKET_SIZE];
    
    // Байт 0: управление динамической индикацией для времени
    packet[TOP_BYTE_CONTROL] = 0x00;
    
    // Бит 7: двоеточие между секундами и минутами
    if (colonEnabled) {
        packet[TOP_BYTE_CONTROL] |= (1 << TOP_COLON_BIT);
    }
    
    // Биты 0-6: данные часов, минут и секунд (матричная система)
    uint8_t yearThousands = (g_topBoardData.year / 1000) % 10;  // Бит 0: тысячная года
    uint8_t hoursTens = hours / 10;                           // Бит 1: часы десятки
    uint8_t hoursUnits = hours % 10;                          // Бит 2: часы единицы
    uint8_t minutesTens = minutes / 10;                         // Бит 3: минуты десятки
    uint8_t minutesUnits = minutes % 10;                         // Бит 4: минуты единицы
    uint8_t secondsTens = seconds / 10;                         // Бит 5: секунды десятки
    uint8_t secondsUnits = seconds % 10;                         // Бит 6: секунды единицы
    
    packet[TOP_BYTE_CONTROL] |= ((yearThousands & 0x01) << TOP_YEAR_THOUSAND_BIT);
    packet[TOP_BYTE_CONTROL] |= ((hoursTens & 0x03) << TOP_HOURS_TENS_BIT);
    packet[TOP_BYTE_CONTROL] |= ((hoursUnits & 0x0F) << TOP_HOURS_UNITS_BIT);
    packet[TOP_BYTE_CONTROL] |= ((minutesTens & 0x07) << TOP_MINUTES_TENS_BIT);
    packet[TOP_BYTE_CONTROL] |= ((minutesUnits & 0x0F) << TOP_MINUTES_UNITS_BIT);
    packet[TOP_BYTE_CONTROL] |= ((secondsTens & 0x07) << TOP_SECONDS_TENS_BIT);
    packet[TOP_BYTE_CONTROL] |= ((secondsUnits & 0x0F) << TOP_SECONDS_UNITS_BIT);
    
    // Байт 1: семисегментные индикаторы времени ЧЧ:ММ:СС
    // Отображаем часы и минуты
    uint8_t h1 = digitCodes[hoursTens];
    uint8_t h2 = digitCodes[hoursUnits];
    uint8_t m1 = digitCodes[minutesTens];
    uint8_t m2 = digitCodes[minutesUnits];
    uint8_t s1 = digitCodes[secondsTens];
    uint8_t s2 = digitCodes[secondsUnits];
    
    // Для простоты используем только первые две цифры времени
    packet[TOP_BYTE_TIME] = h1;
    
    // Байт 2: семисегментные индикаторы даты ГГГГ.ММ.ДД
    // Для режима времени не используется
    packet[TOP_BYTE_DATE] = SEG_OFF;
    
    // Байт 3: день недели и индикатор григорианского календаря
    packet[TOP_BYTE_WEEKDAY] = g_topBoardData.weekday;
    if (g_topBoardData.calendarType == CALENDAR_GREGORIAN) {
        packet[TOP_BYTE_WEEKDAY] |= (1 << TOP_GREGORIAN_LED_BIT);
    }
    
    // Байт 4: месяц Хиджры и индикатор хиджра календаря
    packet[TOP_BYTE_HIJRI] = g_topBoardData.hijriMonth;
    if (g_topBoardData.calendarType == CALENDAR_HIJRI) {
        packet[TOP_BYTE_HIJRI] |= (1 << TOP_HIJRI_LED_BIT);
    }
    
    sendTopBoardData(packet);
}

/**
 * @brief Отображение даты на верхней плате
 */
void displayDate(uint16_t year, uint8_t month, uint8_t day) {
    uint8_t packet[TOP_DATA_PACKET_SIZE];
    
    // Байт 0: управление динамической индикацией (не используется для даты)
    packet[TOP_BYTE_CONTROL] = 0x00;
    
    // Байт 1: семисегментные индикаторы времени ЧЧ:ММ:СС
    // Для режима даты не используется
    packet[TOP_BYTE_TIME] = SEG_OFF;
    
    // Байт 2: семисегментные индикаторы даты ГГГГ.ММ.ДД
    // Биты 0-6: данные даты (матричная система)
    uint8_t yearHundreds = (year / 100) % 10;  // Бит 0: сотая года
    uint8_t yearTens = (year / 10) % 10;      // Бит 1: десятки года
    uint8_t yearUnits = year % 10;             // Бит 2: единицы года
    uint8_t monthTens = month / 10;            // Бит 3: десятки месяца
    uint8_t monthUnits = month % 10;            // Бит 4: единицы месяца
    uint8_t dayTens = day / 10;                // Бит 5: десятки даты
    uint8_t dayUnits = day % 10;                // Бит 6: единицы даты
    
    packet[TOP_BYTE_DATE] = 0x00;
    packet[TOP_BYTE_DATE] |= ((yearHundreds & 0x01) << TOP_DATE_YEAR_HUNDREDS_BIT);
    packet[TOP_BYTE_DATE] |= ((yearTens & 0x0F) << TOP_DATE_YEAR_TENS_BIT);
    packet[TOP_BYTE_DATE] |= ((yearUnits & 0x0F) << TOP_DATE_YEAR_UNITS_BIT);
    packet[TOP_BYTE_DATE] |= ((monthTens & 0x0F) << TOP_DATE_MONTH_TENS_BIT);
    packet[TOP_BYTE_DATE] |= ((monthUnits & 0x0F) << TOP_DATE_MONTH_UNITS_BIT);
    packet[TOP_BYTE_DATE] |= ((dayTens & 0x0F) << TOP_DATE_DAY_TENS_BIT);
    packet[TOP_BYTE_DATE] |= ((dayUnits & 0x0F) << TOP_DATE_DAY_UNITS_BIT);
    
    // Бит 7: двоеточие выключено для даты
    packet[TOP_BYTE_DATE] &= ~(1 << TOP_DATE_COLON_BIT);
    
    // Байт 3: день недели и индикатор григорианского календаря
    packet[TOP_BYTE_WEEKDAY] = g_topBoardData.weekday;
    if (g_topBoardData.calendarType == CALENDAR_GREGORIAN) {
        packet[TOP_BYTE_WEEKDAY] |= (1 << TOP_GREGORIAN_LED_BIT);
    }
    
    // Байт 4: месяц Хиджры и индикатор хиджра календаря
    packet[TOP_BYTE_HIJRI] = g_topBoardData.hijriMonth;
    if (g_topBoardData.calendarType == CALENDAR_HIJRI) {
        packet[TOP_BYTE_HIJRI] |= (1 << TOP_HIJRI_LED_BIT);
    }
    
    sendTopBoardData(packet);
}

/**
 * @brief Динамическая индикация - сканирование анодов 0-7 каждые 500 мс
 */
void multiplexTopBoard() {
    static uint8_t scan = 0;
    unsigned long now = millis();

    if (now - g_lastSwitchTime < SWITCH_INTERVAL) return;
    g_lastSwitchTime = now;

    uint8_t packet[TOP_DATA_PACKET_SIZE];

    uint16_t year = g_topBoardData.year;
    uint8_t month = g_topBoardData.month;
    uint8_t day = g_topBoardData.day;
    uint8_t h = g_topBoardData.hours;
    uint8_t m = g_topBoardData.minutes;
    uint8_t s = g_topBoardData.seconds;

    uint8_t year_th = (year / 1000) % 10;
    uint8_t year_hu = (year / 100) % 10;
    uint8_t year_te = (year / 10) % 10;
    uint8_t year_un = year % 10;

    uint8_t h_t = h / 10;
    uint8_t h_u = h % 10;
    uint8_t m_t = m / 10;
    uint8_t m_u = m % 10;
    uint8_t s_t = s / 10;
    uint8_t s_u = s % 10;

    uint8_t d_t = day / 10;
    uint8_t d_u = day % 10;
    uint8_t mo_t = month / 10;
    uint8_t mo_u = month % 10;

    // byte0 маски анодов (0 = активный)
    static const uint8_t scanMask[8] = {
        0b01111111,
        0b10111111,
        0b11011111,
        0b11101111,
        0b11110111,
        0b11111011,
        0b11111101,
        0b11111110
    };

    packet[TOP_BYTE_CONTROL] = scanMask[scan];

    switch (scan) {
        case 7:
            packet[TOP_BYTE_TIME] = digitCodes[year_th];
            packet[TOP_BYTE_DATE] = digitCodes[year_hu];
            break;

        case 6:
            packet[TOP_BYTE_TIME] = digitCodes[h_t];
            packet[TOP_BYTE_DATE] = digitCodes[year_te];
            break;

        case 5:
            packet[TOP_BYTE_TIME] = digitCodes[h_u];
            packet[TOP_BYTE_DATE] = digitCodes[year_un];
            break;

        case 4:
            packet[TOP_BYTE_TIME] = digitCodes[m_t];
            packet[TOP_BYTE_DATE] = digitCodes[mo_t];
            break;

        case 3:
            packet[TOP_BYTE_TIME] = digitCodes[m_u];
            packet[TOP_BYTE_DATE] = digitCodes[mo_u];
            break;

        case 2:
            packet[TOP_BYTE_TIME] = digitCodes[s_t];
            packet[TOP_BYTE_DATE] = digitCodes[d_t];
            break;

        case 1:
            packet[TOP_BYTE_TIME] = digitCodes[s_u];
            packet[TOP_BYTE_DATE] = digitCodes[d_u];
            break;

        case 0:
            packet[TOP_BYTE_TIME] = SEG_OFF;
            packet[TOP_BYTE_DATE] = SEG_OFF;

            // Моргание двоеточия каждую секунду (включено на четных секундах)
            if (g_topBoardData.colonEnabled && (g_topBoardData.seconds % 2) == 0)
                packet[TOP_BYTE_CONTROL] = 0b01111111; // включить двоеточие
            else
                packet[TOP_BYTE_CONTROL] = 0b11111111; // выключить
            break;
    }

    // weekday + calendar leds
    packet[TOP_BYTE_WEEKDAY] = g_topBoardData.weekday;
    if (g_topBoardData.calendarType == CALENDAR_HIJRI)
        packet[TOP_BYTE_WEEKDAY] |= (1 << TOP_GREGORIAN_LED_BIT);

    packet[TOP_BYTE_HIJRI] = g_topBoardData.hijriMonth;
    if (g_topBoardData.calendarType == CALENDAR_GREGORIAN)
        packet[TOP_BYTE_HIJRI] |= (1 << TOP_HIJRI_LED_BIT);

    sendTopBoardData(packet);

    scan++;
    if (scan > 7) scan = 0;
}