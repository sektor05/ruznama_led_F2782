#ifndef RTC_H
#define RTC_H

#include <Arduino.h>
#include <RTClib.h>
#include <Preferences.h>

// Объявление объекта RTC (определение в rtc.cpp)
extern RTC_DS3231 rtc;

// ============================================
// Константы для Preferences
// ============================================
#define HIJRI_OFFSET_KEY "hijri_offset"

// ============================================
// Получить смещение даты хиджры из Preferences
// ============================================
inline int rtcGetHijriOffset() {
    Preferences prefs;
    prefs.begin("ruznama", false);
    int offset = prefs.getInt(HIJRI_OFFSET_KEY, 0);
    prefs.end();
    return offset;
}

// ============================================
// Установить смещение даты хиджры в Preferences
// ============================================
inline void rtcSetHijriOffset(int offset) {
    Preferences prefs;
    prefs.begin("ruznama", false);
    prefs.putInt(HIJRI_OFFSET_KEY, offset);
    prefs.end();
    Serial.print("RTC: Установлено смещение даты хиджры: ");
    Serial.println(offset);
}

// ============================================
// Инициализация RTC
// ============================================
inline void rtcInit() {
    Serial.print("RTC: Инициализация... ");
    
    // Инициализация RTC
    if (!rtc.begin()) {
        Serial.println("RTC не найден! Проверьте подключение.");
        while (1);
    }
    
    Serial.println("OK");
    
    // Проверяем, работает ли RTC
    if (rtc.lostPower()) {
        Serial.println("RTC: Потеря питания, установка времени компиляции...");
        // Устанавливаем время компиляции
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    
    // Выводим температуру
    float temp = rtc.getTemperature();
    Serial.print("RTC: Температура = ");
    Serial.print(temp);
    Serial.println(" C");
}

// ============================================
// Получить текущее время из RTC
// ============================================
inline DateTime rtcGetTime() {
    return rtc.now();
}

// ============================================
// Установить время в RTC
// ============================================
inline void rtcSetTime(DateTime dateTime) {
    Serial.print("RTC: Установка времени: ");
    Serial.print(dateTime.year());
    Serial.print("-");
    Serial.print(dateTime.month());
    Serial.print("-");
    Serial.print(dateTime.day());
    Serial.print(" ");
    Serial.print(dateTime.hour());
    Serial.print(":");
    Serial.print(dateTime.minute());
    Serial.print(":");
    Serial.println(dateTime.second());
    
    rtc.adjust(dateTime);
    Serial.println("RTC: Время установлено успешно");
}

// ============================================
// Установить время из строки формата "YYYY-MM-DD HH:MM:SS"
// ============================================
inline bool rtcSetTimeFromString(const char* timeString) {
    int year, month, day, hour, minute, second;
    
    // Парсим строку "YYYY-MM-DD HH:MM:SS"
    if (sscanf(timeString, "%d-%d-%d %d:%d:%d", 
               &year, &month, &day, &hour, &minute, &second) == 6) {
        
        DateTime dateTime(year, month, day, hour, minute, second);
        rtcSetTime(dateTime);
        return true;
    }
    
    Serial.println("RTC: Ошибка парсинга времени! Ожидается формат: YYYY-MM-DD HH:MM:SS");
    return false;
}

// ============================================
// Установить время из строки формата "DD.MM.YYYY HH:MM:SS"
// ============================================
inline bool rtcSetTimeFromRussianFormat(const char* timeString) {
    int day, month, year, hour, minute, second;
    
    // Парсим строку "DD.MM.YYYY HH:MM:SS"
    if (sscanf(timeString, "%d.%d.%d %d:%d:%d", 
               &day, &month, &year, &hour, &minute, &second) == 6) {
        
        DateTime dateTime(year, month, day, hour, minute, second);
        rtcSetTime(dateTime);
        return true;
    }
    
    Serial.println("RTC: Ошибка парсинга времени! Ожидается формат: DD.MM.YYYY HH:MM:SS");
    return false;
}

// ============================================
// Получить температуру из RTC
// ============================================
inline float rtcGetTemperature() {
    return rtc.getTemperature();
}

// ============================================
// Вывести текущее время в Serial
// ============================================
inline void rtcPrintTime() {
    DateTime now = rtcGetTime();
    
    Serial.print("RTC: Текущее время: ");
    
    char datestring[26];
    snprintf(datestring, sizeof(datestring),
             "%02u/%02u/%04u %02u:%02u:%02u",
             now.day(),
             now.month(),
             now.year(),
             now.hour(),
             now.minute(),
             now.second());
    
    Serial.println(datestring);
}

// ============================================
// Структура для хранения даты хиджры
// ============================================
struct HijriDate {
    int day;
    int month;
    int year;
};

// ============================================
// Получить время как строку
// ============================================
inline String rtcGetTimeString() {
    DateTime now = rtcGetTime();
    
    char datestring[26];
    snprintf(datestring, sizeof(datestring),
             "%02u/%02u/%04u %02u:%02u:%02u",
             now.day(),
             now.month(),
             now.year(),
             now.hour(),
             now.minute(),
             now.second());
    
    return String(datestring);
}

// ============================================
// Конвертировать григорианскую дату в хиджру
// ============================================
inline HijriDate gregorianToHijri(int gy, int gm, int gd) {
    // Алгоритм конвертации Григорианского календаря в Хиджру
    // Основан на формуле из библиотеки HijriDate
    
    // 1. Вычисляем Julian Day (JD)
    if (gm < 3) {
        gy -= 1;
        gm += 12;
    }
    
    int a = (int)(gy / 100);
    int b = 2 - a + (int)(a / 4);
    
    long jd = (long)(365.25 * (gy + 4716)) + (long)(30.6001 * (gm + 1)) + gd + b - 1524;

    // 2. Конвертируем JD в дату Хиджры
    // Эпоха Хиджры (JD 1948439.5)
    long l = jd - 1948440 + 10632;
    int n = (int)((l - 1) / 10631);
    l = l - 10631 * n + 354;
    
    int j = ((int)((10985 - l) / 5316)) * ((int)((50 * l) / 17719)) + ((int)(l / 5670)) * ((int)((43 * l) / 15238));
    l = l - ((int)((30 - j) / 15)) * ((int)((17719 * j) / 50)) - ((int)(j / 16)) * ((int)((15238 * j) / 43)) + 29;
    
    int m = (int)((24 * l) / 709);
    int d = l - (int)((709 * m) / 24);
    int y = 30 * n + j - 30;

    HijriDate hijri;
    hijri.day = d;
    hijri.month = m;
    hijri.year = y;
    
    return hijri;
}

// ============================================
// Получить текущую дату хиджры с учетом смещения
// ============================================
inline HijriDate rtcGetHijriDate() {
    DateTime now = rtcGetTime();
    
    // Получаем сохраненное смещение
    int offsetDays = rtcGetHijriOffset();
    
    // Добавляем смещение к текущей дате
    DateTime adjustedDate = now + TimeSpan(offsetDays, 0, 0, 0);
    
    return gregorianToHijri(adjustedDate.year(), adjustedDate.month(), adjustedDate.day());
}

// ============================================
// Получить дату хиджры как строку
// ============================================
inline String rtcGetHijriDateString() {
    HijriDate hijri = rtcGetHijriDate();
    
    char hijriStr[30];
    snprintf(hijriStr, sizeof(hijriStr), "%d.%d.%d AH", 
             hijri.day, hijri.month, hijri.year);
    
    return String(hijriStr);
}

// ============================================
// Получить дату хиджры как JSON
// ============================================
inline String rtcGetHijriDateJSON() {
    HijriDate hijri = rtcGetHijriDate();
    
    char hijriStr[50];
    snprintf(hijriStr, sizeof(hijriStr), 
             "{\"day\": %d, \"month\": %d, \"year\": %d}", 
             hijri.day, hijri.month, hijri.year);
    
    return String(hijriStr);
}

#endif // RTC_H
