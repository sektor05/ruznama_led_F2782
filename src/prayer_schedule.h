#ifndef PRAYER_SCHEDULE_H
#define PRAYER_SCHEDULE_H

#include <Arduino.h>
#include <RTClib.h>

// Структура для хранения времени одной молитвы
struct PrayerTime {
    String name;    // Название молитвы (Фаджр, Восход, Зухр, Аср, Магриб, Иша)
    uint8_t hour;   // Часы
    uint8_t minute; // Минуты
};

// Структура для хранения расписания на один день
struct DailySchedule {
    String date;           // Дата в формате "YYYY-MM-DD"
    String month;          // Название месяца
    PrayerTime prayers[6]; // Массив из 6 молитв
};

// Класс для управления расписанием молитв
class PrayerSchedule {
public:
    PrayerSchedule();
    
    // Инициализация файловой системы
    bool begin();
    
    // Загрузка расписания на конкретную дату
    bool loadSchedule(const String& date);
    
    // Получение расписания на текущую дату
    bool loadCurrentSchedule();
    
    // Получить время молитвы по индексу (0-5)
    PrayerTime getPrayerTime(uint8_t index) const;
    
    // Получить время молитвы по названию
    PrayerTime getPrayerTime(const String& name) const;
    
    // Получить текущее расписание
    const DailySchedule& getCurrentSchedule() const;
    
    // Проверить, загружено ли расписание
    bool isLoaded() const;
    
    // Получить следующую молитву после указанного времени
    PrayerTime getNextPrayer(uint8_t hour, uint8_t minute) const;
    
    // Получить время икамата для молитвы по индексу (0-5)
    PrayerTime getIqamahTime(uint8_t index) const;
    
    // Получить время икамата для молитвы по названию
    PrayerTime getIqamahTime(const String& name) const;
    
    // Получить время до следующего икамата в минутах
    // Возвращает -1 если икамат уже прошел или не определен
    int getMinutesToIqamah(uint8_t hour, uint8_t minute, uint8_t dayOfWeek) const;
    
    // Получить время до следующего икамата в секундах
    // Возвращает -1 если икамат уже прошел или не определен
    int getSecondsToIqamah(uint8_t hour, uint8_t minute, uint8_t second, uint8_t dayOfWeek) const;
    
    // Проверить, является ли текущая молитва джума (пятничный намаз)
    bool isJumaPrayer(const String& prayerName, uint8_t dayOfWeek) const;
    
    // Получить индекс молитвы, время которой наступило в текущую минуту
    // Возвращает индекс (0-5) если время молитвы совпадает с текущим временем
    // Возвращает -1 если ни одна молитва не наступила
    int getCurrentPrayerIndex(uint8_t hour, uint8_t minute) const;

private:
    DailySchedule currentSchedule;
    bool scheduleLoaded;
    
    // Парсинг времени из строки "HH:MM"
    bool parseTime(const String& timeStr, uint8_t& hour, uint8_t& minute);
    
    // Форматирование даты в строку "YYYY-MM-DD"
    String formatDate(uint16_t year, uint8_t month, uint8_t day) const;
    
    // Форматирование даты без года (только месяц и день) "--MM-DD"
    String formatDateNoYear(uint8_t month, uint8_t day) const;
};

#endif // PRAYER_SCHEDULE_H
