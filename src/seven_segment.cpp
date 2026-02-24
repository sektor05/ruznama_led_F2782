#include "seven_segment.h"

// ============================================
// Глобальные данные для мультиплексирования
// ============================================
static DisplayData g_displayDataBottom;  // Данные для нижней платы
static DisplayData g_displayDataTop;      // Данные для верхней платы
static int g_currentMultiplexRow = 0;    // Текущая строка для мультиплексирования

// ============================================
// Медленная функция shiftOut с задержками
// ============================================
void slowShiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val) {
    for (uint8_t i = 0; i < 8; i++)  {
        uint8_t bit;
        if (bitOrder == LSBFIRST) bit = !!(val & (1 << i));
        else bit = !!(val & (1 << (7 - i)));

        digitalWrite(dataPin, bit);
        //delayMicroseconds(9); // Даем сигналу стабилизироваться
        digitalWrite(clockPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(clockPin, LOW);
    }
}

// ============================================
// Массив кодов для цифр 0-9
// ============================================
static const uint8_t digitCodes[10] = {
    SEG_0, SEG_1, SEG_2, SEG_3, SEG_4,
    SEG_5, SEG_6, SEG_7, SEG_8, SEG_9
};

// ============================================
// Функция для переворота сегментов на 180 градусов
// Меняет местами сегменты: a<->d, b<->e, c<->f
// ============================================
static uint8_t flipSegments180(uint8_t code) {
    uint8_t result = 0;
    
    // Биты: 0=a, 1=b, 2=c, 3=d, 4=e, 5=f, 6=g, 7=dp
    // После поворота на 180 градусов:
    // a (бит0) <-> d (бит3)
    // b (бит1) <-> e (бит4)
    // c (бит2) <-> f (бит5)
    // g (бит6) остается без изменений
    // dp (бит7) остается без изменений
    
    // Копируем биты с новыми позициями
    result |= ((code >> 0) & 1) << 6;  // a -> d (бит0 -> бит2)
    result |= ((code >> 1) & 1) << 5;  // b -> e (бит1 -> бит5)
    result |= ((code >> 2) & 1) << 4;  // c -> f (бит2 -> бит0)
    result |= ((code >> 3) & 1) << 3;  // d -> a (бит3 -> бит3)
    result |= ((code >> 4) & 1) << 2;  // e -> b (бит4 -> бит6)
    result |= ((code >> 5) & 1) << 1;  // f -> c (бит5 -> бит1)
    result |= ((code >> 6) & 1) << 0;  // g -> g (бит6 -> бит4)
    result |= ((code >> 7) & 1) << 7;  // dp -> dp (бит7 -> бит7)
    
    return result;
}

// ============================================
// Инициализация драйвера семисегментного индикатора
// ============================================
void sevenSegmentInit() {
    // Пины уже инициализированы в boardInit()
    // Эта функция оставлена для совместимости и возможного расширения
}

// ============================================
// Получение кода для цифры с точкой
// ============================================
static uint8_t getDigitCode(int digit, bool withDot, bool flip = false) {
    uint8_t code = digitCodes[digit];
    
#if SEGMENT_TYPE == SEGMENT_TYPE_COMMON_ANODE
    // Общий анод: точка включена, когда бит 7 = 0
    if (withDot) {
        code &= 0x7F;  // Сбросить бит 7 (включить точку)
    } else {
        code |= 0x80;  // Установить бит 7 (выключить точку)
    }
#else
    // Общий катод: точка включена, когда бит 7 = 1
    if (withDot) {
        code |= 0x80;  // Установить бит 7 (включить точку)
    } else {
        code &= 0x7F;  // Сбросить бит 7 (выключить точку)
    }
#endif
    
    if (flip) {
        // Переворачиваем сегменты на 180 градусов
        code = flipSegments180(code);
    }
    
    return code;
}

// ============================================
// Получение кода для ряда (строки)
// ============================================
static uint8_t getRowCode(int row, bool withColon = false) {
    // Ряды: 1-7
    // Формат байта управления рядами:
    // Бит 7: двоеточие (1 = включено, 0 = выключено)
    // Бит 6: строка 7
    // Бит 5: строка 1
    // Бит 4: строка 2
    // Бит 3: строка 3
    // Бит 2: строка 4
    // Бит 1: строка 5
    // Бит 0: строка 6

    // Создаем маску для активации одного ряда
    // Активный ряд: 0 = включен, 1 = выключен (для общего анода)
    // Или 1 = включен, 0 = выключен (для общего катода)

    uint8_t rowMask = 0xFF;  // По умолчанию все ряды выключены

    // Устанавливаем бит для активного ряда
    // Строка 1 -> бит 5
    // Строка 2 -> бит 4
    // Строка 3 -> бит 3
    // Строка 4 -> бит 2
    // Строка 5 -> бит 1
    // Строка 6 -> бит 0
    // Строка 7 -> бит 6
    switch (row) {
        case 1: rowMask = ~(1 << 5); break;  // Строка 1 -> бит 5
        case 2: rowMask = ~(1 << 4); break;  // Строка 2 -> бит 4
        case 3: rowMask = ~(1 << 3); break;  // Строка 3 -> бит 3
        case 4: rowMask = ~(1 << 2); break;  // Строка 4 -> бит 2
        case 5: rowMask = ~(1 << 1); break;  // Строка 5 -> бит 1
        case 6: rowMask = ~(1 << 0); break;  // Строка 6 -> бит 0
        case 7: rowMask = ~(1 << 6); break;  // Строка 7 -> бит 6
        default: rowMask = 0xFF; break;      // Все ряды выключены
    }

    // Обрабатываем двоеточие (бит 7)
#if SEGMENT_TYPE == SEGMENT_TYPE_COMMON_ANODE
    // Общий анод: 0 = включено
    if (withColon) {
        rowMask &= 0x7F;  // Сбросить бит 7 (включить двоеточие)
    } else {
        rowMask |= 0x80;  // Установить бит 7 (выключить двоеточие)
    }
    return rowMask;
#else
    // Общий катод: 1 = включено
    if (withColon) {
        rowMask |= 0x80;  // Установить бит 7 (включить двоеточие)
    } else {
        rowMask &= 0x7F;  // Сбросить бит 7 (выключить двоеточие)
    }
    return ~rowMask;
#endif
}

// ============================================
// Отправка массива данных в регистры 74HC595
// Порядок отправки: [0]->[4] (первый байт уходит в 1-ю микросхему)
// В соответствии с testShiftOutByte():
// [0] — 1-я микросхема (Цифра 3 - десятки минут)
// [1] — 2-я микросхема (Цифра 4 - единицы минут)
// [2] — 3-я микросхема (Ряды с двоеточием)
// [3] — 4-я микросхема (Цифра 1 - десятки часов)
// [4] — 5-я микросхема (Цифра 2 - единицы часов)
// ============================================
void shiftOutData(uint8_t data[NUM_REGISTERS], BoardType boardType) {
    // Определяем пины в зависимости от типа платы
    uint8_t dataPin, latchPin, clockPin;
    
    if (boardType == BOARD_TOP) {
        dataPin = PIN_595_DATA_TOP;
        latchPin = PIN_595_LATCH_TOP;
        clockPin = PIN_595_CLOCK_TOP;
    } else {
        dataPin = PIN_595_DATA_BOT;
        latchPin = PIN_595_LATCH_BOT;
        clockPin = PIN_595_CLOCK_BOT;
    }
    
    // Опускаем LATCH для подготовки к приему данных
    digitalWrite(latchPin, LOW);
    delayMicroseconds(1);  // Небольшая задержка для стабилизации
    
    // Отправляем байты в порядке: [0] -> [4]
    // Первый байт "пролетает" через всю цепочку и оказывается в 5-й микросхеме
    for (int i = 0; i < NUM_REGISTERS; i++) {
        uint8_t byteToSend = data[i];
#if DATA_INVERTED
        byteToSend = ~byteToSend;  // Инвертируем данные, если используется инвертор
#endif
#if BIT_ORDER == BIT_ORDER_LSBFIRST
        slowShiftOut(dataPin, clockPin, LSBFIRST, byteToSend);
#else
        slowShiftOut(dataPin, clockPin, MSBFIRST, byteToSend);
#endif
    }
    
    // Поднимаем LATCH для фиксации данных в выходных регистрах
    digitalWrite(latchPin, HIGH);
    delayMicroseconds(1);  // Задержка для фиксации данных (минимум 1 мкс для 74HC595)
}

// ============================================
// Отображение данных (времени) на указанном ряду
// ============================================
void displayData(BoardType boardType, int hours, int minutes, int row) {
    // Проверка диапазонов
    if (hours < 0 || hours > 23) hours = 0;
    if (minutes < 0 || minutes > 59) minutes = 0;
    if (row < 1 || row > NUM_ROWS) row = 1;
    
    // Разбираем часы и минуты на цифры
    int h1 = hours / 10;       // Десятки часов
    int h2 = hours % 10;       // Единицы часов
    int m1 = minutes / 10;     // Десятки минут
    int m2 = minutes % 10;     // Единицы минут
    
    // Определяем, нужно ли повернуть единицы часов на 180 градусов
    // Поворачиваем на строках: 2, 4, 5, 6, 7
    bool flipHoursUnits = (row == 2 || row == 4 || row == 5 || row == 6 || row == 7);
    
    // Определяем, нужно ли повернуть десятки часов на 180 градусов
    // Поворачиваем на строках: 2, 3, 4, 5, 6, 7
    bool flipHoursTens = (row == 2 || row == 3 || row == 4 || row == 5 || row == 6 || row == 7);
    
    // Формируем массив данных для отправки
    // Порядок отправки: [0]->[4] (первый байт уходит в 1-ю микросхему)
    // В соответствии с testShiftOutByte():
    // [0] = Цифра3 (десятки минут)
    // [1] = Цифра4 (единицы минут)
    // [2] = Ряды (с двоеточием)
    // [3] = Цифра1 (десятки часов)
    // [4] = Цифра2 (единицы часов)
    uint8_t data[NUM_REGISTERS];

    // [0] — 1-я микросхема (Цифра 3) - десятки минут
    data[0] = getDigitCode(m1, false, false);

    // [1] — 2-я микросхема (Цифра 4) - единицы минут
    data[1] = getDigitCode(m2, false, false);

    // [2] — 3-я микросхема (Ряды с двоеточием)
    data[2] = getRowCode(row, true);  // Включаем двоеточие

    // [3] — 4-я микросхема (Цифра 1) - десятки часов
    data[3] = getDigitCode(h1, false, flipHoursTens);

    // [4] — 5-я микросхема (Цифра 2) - единицы часов (развернуть на 180 градусов для строк 2, 4, 5, 6, 7)
    data[4] = getDigitCode(h2, false, flipHoursUnits);
    
    // Отправляем данные в регистры
    shiftOutData(data, boardType);
}

// ============================================
// Отображение данных (времени) на указанном ряду (нижняя плата)
// ============================================
void displayDataBottom(int hours, int minutes, int row) {
    displayData(BOARD_BOTTOM, hours, minutes, row);
}

// ============================================
// Очистка всех индикаторов на указанной плате
// ============================================
void clearDisplay(BoardType boardType) {
    // Формируем массив с выключенными сегментами
    uint8_t data[NUM_REGISTERS];
    
    for (int i = 0; i < NUM_REGISTERS; i++) {
        data[i] = SEG_OFF;
    }
    
    // Отправляем данные в регистры
    shiftOutData(data, boardType);
}

// ============================================
// Очистка всех индикаторов на нижней плате
// ============================================
void clearDisplayBottom() {
    clearDisplay(BOARD_BOTTOM);
}

// ============================================
// Функции для динамической индикации намазов
// ============================================

/**
 * @brief Инициализация данных отображения
 */
void initDisplayData() {
    // Инициализируем данные для нижней платы
    for (int i = 0; i < 7; i++) {
        g_displayDataBottom.rows[i].digit1 = 0;
        g_displayDataBottom.rows[i].digit2 = 0;
        g_displayDataBottom.rows[i].digit3 = 0;
        g_displayDataBottom.rows[i].digit4 = 0;
        g_displayDataBottom.rows[i].withColon = false;
        g_displayDataBottom.rows[i].active = false;
    }
    
    // Инициализируем данные для верхней платы
    for (int i = 0; i < 7; i++) {
        g_displayDataTop.rows[i].digit1 = 0;
        g_displayDataTop.rows[i].digit2 = 0;
        g_displayDataTop.rows[i].digit3 = 0;
        g_displayDataTop.rows[i].digit4 = 0;
        g_displayDataTop.rows[i].withColon = false;
        g_displayDataTop.rows[i].active = false;
    }
}

/**
 * @brief Установка данных для строки
 */
void setRowData(int row, int hours, int minutes, bool withColon) {
    // Проверка диапазонов
    if (row < 1 || row > 7) return;
    // Изменено: разрешаем значения до 99 (для отображения секунд/минут до икамата)
    if (hours < 0 || hours > 99) hours = 0;
    if (minutes < 0 || minutes > 59) minutes = 0;
    
    int rowIndex = row - 1;  // Преобразуем в индекс массива (0-6)
    
    // Разбираем часы и минуты на цифры
    int h1 = hours / 10;       // Десятки часов
    int h2 = hours % 10;       // Единицы часов
    int m1 = minutes / 10;     // Десятки минут
    int m2 = minutes % 10;     // Единицы минут
    
    // Устанавливаем данные для нижней платы
    g_displayDataBottom.rows[rowIndex].digit1 = h1;
    g_displayDataBottom.rows[rowIndex].digit2 = h2;
    g_displayDataBottom.rows[rowIndex].digit3 = m1;
    g_displayDataBottom.rows[rowIndex].digit4 = m2;
    g_displayDataBottom.rows[rowIndex].withColon = withColon;
    g_displayDataBottom.rows[rowIndex].active = true;
}

/**
 * @brief Очистка данных строки
 */
void clearRowData(int row) {
    // Проверка диапазонов
    if (row < 1 || row > 7) return;
    
    int rowIndex = row - 1;  // Преобразуем в индекс массива (0-6)
    
    // Очищаем данные для нижней платы
    g_displayDataBottom.rows[rowIndex].active = false;
}

/**
 * @brief Мультиплексированное отображение всех строк
 * 
 * Эта функция должна вызываться постоянно в loop() для создания эффекта
 * одновременного отображения всех строк
 */
void multiplexDisplay(BoardType boardType) {
    // Выбираем данные для нужной платы
    DisplayData* displayData = (boardType == BOARD_TOP) ? &g_displayDataTop : &g_displayDataBottom;
    
    // Переключаемся на следующую строку
    g_currentMultiplexRow++;
    if (g_currentMultiplexRow > 7) {
        g_currentMultiplexRow = 1;
    }
    
    int rowIndex = g_currentMultiplexRow - 1;  // Преобразуем в индекс массива (0-6)
    
    // Проверяем, активна ли строка
    if (!displayData->rows[rowIndex].active) {
        // Если строка не активна, пропускаем её
        return;
    }
    
    // Определяем, нужно ли повернуть единицы часов на 180 градусов
    // Поворачиваем на строках: 2, 4, 5, 6, 7
    bool flipHoursUnits = (g_currentMultiplexRow == 2 ||
                           g_currentMultiplexRow == 4 ||
                           g_currentMultiplexRow == 5 ||
                           g_currentMultiplexRow == 6 ||
                           g_currentMultiplexRow == 7);
    
    // Определяем, нужно ли повернуть десятки часов на 180 градусов
    // Поворачиваем на строках: 2, 3, 4, 5, 6, 7
    bool flipHoursTens = (g_currentMultiplexRow == 2 ||
                          g_currentMultiplexRow == 3 ||
                          g_currentMultiplexRow == 4 ||
                          g_currentMultiplexRow == 5 ||
                          g_currentMultiplexRow == 6 ||
                          g_currentMultiplexRow == 7);
    
    // Формируем массив данных для отправки
    uint8_t data[NUM_REGISTERS];
    
    // [0] — 1-я микросхема (Цифра 3) - десятки минут
    data[0] = getDigitCode(displayData->rows[rowIndex].digit3, false, false);
    
    // [1] — 2-я микросхема (Цифра 4) - единицы минут
    data[1] = getDigitCode(displayData->rows[rowIndex].digit4, false, false);
    
    // [2] — 3-я микросхема (Ряды с двоеточием)
    data[2] = getRowCode(g_currentMultiplexRow, displayData->rows[rowIndex].withColon);
    
    // [3] — 4-я микросхема (Цифра 1) - десятки часов
    data[3] = getDigitCode(displayData->rows[rowIndex].digit1, false, flipHoursTens);
    
    // [4] — 5-я микросхема (Цифра 2) - единицы часов (развернуть на 180 градусов для строк 2, 4, 5, 6, 7)
    data[4] = getDigitCode(displayData->rows[rowIndex].digit2, false, flipHoursUnits);
    
    // Отправляем данные в регистры
    shiftOutData(data, boardType);
}

/**
 * @brief Отображение всех молитв на соответствующих строках (1-6)
 */
void displayAllPrayers(BoardType boardType, const DailySchedule& schedule) {
    // Отображаем каждую молитву на своей строке
    for (int i = 0; i < 6; i++) {
        PrayerTime prayer = schedule.prayers[i];
        // Строки 1-6 соответствуют молитвам 0-5
        setRowData(i + 1, prayer.hour, prayer.minute, true);
    }
}

/**
 * @brief Отображение текущего времени и минут до икамата на 7-й строке
 * 
 * Формат отображения:
 * - Если minutesToIqamah >= 0: отображаем минуты до икамата (например: "10")
 * - Если minutesToIqamah < 0: отображаем текущее время (например: "14:30")
 */
void displayTimeWithIqamah(BoardType boardType, int hours, int minutes, int minutesToIqamah) {
    if (minutesToIqamah >= 0) {
        // Отображаем минуты до икамата на 7-й строке
        displayMinutesToIqamah(boardType, minutesToIqamah);
    } else {
        // Отображаем текущее время на 7-й строке
        displayCurrentTime(boardType, hours, minutes);
    }
}

/**
 * @brief Отображение только текущего времени на 7-й строке
 */
void displayCurrentTime(BoardType boardType, int hours, int minutes) {
    // Проверка диапазонов
    if (hours < 0 || hours > 23) hours = 0;
    if (minutes < 0 || minutes > 59) minutes = 0;
    
    // Устанавливаем данные для 7-й строки
    // Единицы часов (digit2) будут развернуты на 180 градусов
    setRowData(7, hours, minutes, true);
}

/**
 * @brief Отображение минут до икамата на 7-й строке
 * 
 * Формат: отображаем число минут в области часов (например: "10")
 * Если минут больше 99, отображаем "99"
 */
void displayMinutesToIqamah(BoardType boardType, int minutesToIqamah) {
    // Ограничиваем максимум 99 минут
    if (minutesToIqamah > 99) minutesToIqamah = 99;
    if (minutesToIqamah < 0) minutesToIqamah = 0;
    
    // Устанавливаем данные для 7-й строки (минуты отображаем в области часов)
    // Например: 10 минут -> 10:00 (минуты в digit1 и digit2)
    setRowData(7, minutesToIqamah, 0, false);
}

/**
 * @brief Очистка 7-й строки (отключение индикации)
 */
void clearRow7(BoardType boardType) {
    // Очищаем данные для 7-й строки
    clearRowData(7);
}

/**
 * @brief Отображение времени до икамата на 7-й строке с миганием
 *
 * @param boardType Тип платы (BOARD_TOP или BOARD_BOTTOM)
 * @param timeToIqamah Время до икамата (минуты или секунды)
 * @param blinkState Состояние мигания (true = включено, false = выключено)
 */
void displayToIqamah(BoardType boardType, int timeToIqamah, bool blinkState) {
    // Если blinkState = false, не отображаем ничего (мигание в выключенном состоянии)
    if (!blinkState) {
        clearRowData(7);
        return;
    }

    // Ограничиваем максимум 59 секунд
    if (timeToIqamah > 59) timeToIqamah = 59;
    if (timeToIqamah < 0) timeToIqamah = 0;

    // Устанавливаем данные для 7-й строки напрямую
    // Это позволяет отображать секунды (0-59) в области часов
    setRowData(7, timeToIqamah, 0, false);
}

/**
 * @brief Отображение времени молитвы на указанной строке с миганием
 *
 * @param boardType Тип платы (BOARD_TOP или BOARD_BOTTOM)
 * @param row Номер строки (1-6)
 * @param hours Часы
 * @param minutes Минуты
 * @param blinkState Состояние мигания (true = включено, false = выключено)
 */
void displayPrayerTimeBlink(BoardType boardType, int row, int hours, int minutes, bool blinkState) {
    // Если blinkState = false, не отображаем ничего (мигание в выключенном состоянии)
    if (!blinkState) {
        clearRowData(row);
        return;
    }

    // Проверка диапазонов
    if (row < 1 || row > 6) return;
    if (hours < 0 || hours > 23) hours = 0;
    if (minutes < 0 || minutes > 59) minutes = 0;

    // Устанавливаем данные для указанной строки
    setRowData(row, hours, minutes, true);
}
