#include "prayer_schedule.h"
#include "rtc.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

// Размер JSON документа для парсинга (учитывая размер файла ~241KB)
// Используем динамическое выделение памяти
#define JSON_DOC_SIZE 4096

PrayerSchedule::PrayerSchedule() : scheduleLoaded(false) {
    // Инициализация структуры расписания
    for (int i = 0; i < 6; i++) {
        currentSchedule.prayers[i].name = "";
        currentSchedule.prayers[i].hour = 0;
        currentSchedule.prayers[i].minute = 0;
    }
}

bool PrayerSchedule::begin() {
    // Инициализация файловой системы LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("Ошибка инициализации LittleFS");
        return false;
    }
    
    Serial.println("LittleFS инициализирована успешно");
    
    // Проверка наличия файла
    if (!LittleFS.exists("/year_schedule.json")) {
        Serial.println("Файл year_schedule.json не найден");
        return false;
    }
    
    Serial.println("Файл year_schedule.json найден");
    return true;
}

bool PrayerSchedule::parseTime(const String& timeStr, uint8_t& hour, uint8_t& minute) {
    // Формат времени: "HH:MM"
    int colonIndex = timeStr.indexOf(':');
    if (colonIndex == -1) {
        return false;
    }
    
    hour = timeStr.substring(0, colonIndex).toInt();
    minute = timeStr.substring(colonIndex + 1).toInt();
    
    return (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59);
}

String PrayerSchedule::formatDate(uint16_t year, uint8_t month, uint8_t day) const {
    char dateStr[11];
    sprintf(dateStr, "%04d-%02d-%02d", year, month, day);
    return String(dateStr);
}

// Форматирование даты без года (только месяц и день)
String PrayerSchedule::formatDateNoYear(uint8_t month, uint8_t day) const {
    char dateStr[7]; // "--MM-DD" + null terminator = 7 символов
    sprintf(dateStr, "--%02d-%02d", month, day);
    return String(dateStr);
}

bool PrayerSchedule::loadSchedule(const String& date) {
    // Открытие файла
    File file = LittleFS.open("/year_schedule.json", "r");
    if (!file) {
        Serial.println("Ошибка открытия файла year_schedule.json");
        return false;
    }
    
    Serial.println("Загрузка расписания для даты: " + date);
    
    // Получаем размер файла
    size_t fileSize = file.size();
    Serial.println("Размер файла: " + String(fileSize) + " байт");
    
    // Выделяем память для всего файла
    char* fileContent = new char[fileSize + 1];
    if (!fileContent) {
        Serial.println("Ошибка выделения памяти для файла");
        file.close();
        return false;
    }
    
    // Читаем весь файл
    file.readBytes(fileContent, fileSize);
    fileContent[fileSize] = '\0'; // Добавляем null terminator
    file.close();
    
    Serial.println("Файл прочитан, поиск даты...");
    
    // Парсим весь JSON файл
    // Увеличиваем размер документа для всего файла
    DynamicJsonDocument doc(fileSize + 1024);
    DeserializationError error = deserializeJson(doc, fileContent);
    
    delete[] fileContent; // Освобождаем память
    
    if (error) {
        Serial.print("Ошибка парсинга JSON: ");
        Serial.println(error.c_str());
        return false;
    }
    
    Serial.println("JSON успешно распарсен");
    
    // Ищем нужную дату в массиве
    JsonArray schedules = doc.as<JsonArray>();
    bool found = false;
    int index = 0;
    
    for (auto schedule : schedules) {
        String jsonDate = schedule["date"].as<String>();
        // Сравниваем только месяц и день
        // jsonDate: "2025-02-16", jsonDate.substring(5): "02-16"
        // date: "--02-16", date.substring(2): "02-16"
        String jsonMonthDay = jsonDate.substring(5);
        String searchMonthDay = date.substring(2);
        
        Serial.println("Запись " + String(index) + ": jsonDate=" + jsonDate + ", jsonMonthDay=" + jsonMonthDay + ", searchMonthDay=" + searchMonthDay);
        
        if (jsonMonthDay == searchMonthDay) {
            found = true;
            Serial.println("Дата найдена в записи " + String(index));
            
            // Заполняем структуру расписания
            currentSchedule.date = schedule["date"].as<String>();
            currentSchedule.month = schedule["month"].as<String>();
            
            JsonArray events = schedule["events"];
            for (int i = 0; i < events.size() && i < 6; i++) {
                currentSchedule.prayers[i].name = events[i]["event"].as<String>();
                String timeStr = events[i]["time"].as<String>();
                parseTime(timeStr, currentSchedule.prayers[i].hour, currentSchedule.prayers[i].minute);
            }
            
            break;
        }
        index++;
    }
    
    if (!found) {
        Serial.println("Дата не найдена в расписании");
        return false;
    }
    
    scheduleLoaded = true;
    
    Serial.println("Расписание загружено успешно:");
    Serial.println("Дата: " + currentSchedule.date);
    Serial.println("Месяц: " + currentSchedule.month);
    for (int i = 0; i < 6; i++) {
        Serial.printf("%s: %02d:%02d\n", 
            currentSchedule.prayers[i].name.c_str(),
            currentSchedule.prayers[i].hour,
            currentSchedule.prayers[i].minute);
    }
    
    return true;
}

bool PrayerSchedule::loadCurrentSchedule() {
    DateTime now = rtcGetTime();
    // Используем дату без года для сравнения (только месяц и день)
    String date = formatDateNoYear(now.month(), now.day());
    return loadSchedule(date);
}

PrayerTime PrayerSchedule::getPrayerTime(uint8_t index) const {
    if (index < 6) {
        return currentSchedule.prayers[index];
    }
    PrayerTime empty = {"", 0, 0};
    return empty;
}

PrayerTime PrayerSchedule::getPrayerTime(const String& name) const {
    for (int i = 0; i < 6; i++) {
        if (currentSchedule.prayers[i].name == name) {
            return currentSchedule.prayers[i];
        }
    }
    PrayerTime empty = {"", 0, 0};
    return empty;
}

const DailySchedule& PrayerSchedule::getCurrentSchedule() const {
    return currentSchedule;
}

bool PrayerSchedule::isLoaded() const {
    return scheduleLoaded;
}

PrayerTime PrayerSchedule::getNextPrayer(uint8_t hour, uint8_t minute) const {
    // Преобразуем текущее время в минуты
    int currentMinutes = hour * 60 + minute;
    
    // Ищем следующую молитву
    for (int i = 0; i < 6; i++) {
        int prayerMinutes = currentSchedule.prayers[i].hour * 60 + currentSchedule.prayers[i].minute;
        if (prayerMinutes > currentMinutes) {
            return currentSchedule.prayers[i];
        }
    }
    
    // Если следующей молитвы сегодня нет, возвращаем первую молитву следующего дня
    PrayerTime empty = {"", 0, 0};
    return empty;
}

PrayerTime PrayerSchedule::getIqamahTime(uint8_t index) const {
    if (index >= 6 || !scheduleLoaded) {
        PrayerTime empty = {"", 0, 0};
        return empty;
    }
    
    PrayerTime prayer = currentSchedule.prayers[index];
    PrayerTime iqamah;
    iqamah.name = prayer.name + " (Икамат)";
    
    int prayerMinutes = prayer.hour * 60 + prayer.minute;
    int iqamahMinutes;
    
    // Особая логика для вечернего намаза (Магриб)
    if (prayer.name == "Магриб") {
        // Если время до 17:30, то 5 минут до икамата
        if (prayer.hour < 17 || (prayer.hour == 17 && prayer.minute < 30)) {
            iqamahMinutes = prayerMinutes + 5;
        } else {
            // Если время после 17:30, то 2 минуты до икамата
            iqamahMinutes = prayerMinutes + 2;
        }
    } 
    // Особая логика для обеденного намаза (Зухр)
    else if (prayer.name == "Зухр") {
        // Если время до 12:00, то икамат в 12:10
        if (prayer.hour < 12) {
            iqamahMinutes = 12 * 60 + 10; // 12:10
        } else {
            // Если время 12:00 или позже, то икамат через 10 минут
            iqamahMinutes = prayerMinutes + 10;
        }
    }
    // Все остальные намазы - 10 минут до икамата
    else {
        iqamahMinutes = prayerMinutes + 10;
    }
    
    iqamah.hour = iqamahMinutes / 60;
    iqamah.minute = iqamahMinutes % 60;
    
    return iqamah;
}

PrayerTime PrayerSchedule::getIqamahTime(const String& name) const {
    for (int i = 0; i < 6; i++) {
        if (currentSchedule.prayers[i].name == name) {
            return getIqamahTime(i);
        }
    }
    PrayerTime empty = {"", 0, 0};
    return empty;
}

int PrayerSchedule::getMinutesToIqamah(uint8_t hour, uint8_t minute, uint8_t dayOfWeek) const {
    if (!scheduleLoaded) {
        return -1;
    }
    
    // Пятница - день недели 5 (0=воскресенье, 1=понедельник, ..., 5=пятница, 6=суббота)
    // В пятницу не показываем время до икамата для джума (обеденного намаза)
    bool isFriday = (dayOfWeek == 5);
    
    // Преобразуем текущее время в минуты
    int currentMinutes = hour * 60 + minute;
    
    // Ищем ближайший икамат, но только ПОСЛЕ азана
    for (int i = 0; i < 6; i++) {
        PrayerTime prayer = currentSchedule.prayers[i];
        int prayerMinutes = prayer.hour * 60 + prayer.minute;
        
        // Если это пятница и обеденный намаз (Зухр), пропускаем
        if (isFriday && prayer.name == "Зухр") {
            continue;
        }
        
        // Получаем время икамата для этой молитвы
        PrayerTime iqamah = getIqamahTime(i);
        int iqamahMinutes = iqamah.hour * 60 + iqamah.minute;
        
        // Отчет времени до икамата начинается только ПОСЛЕ азана
        // Текущее время должно быть >= времени азана и < времени икамата
        if (currentMinutes >= prayerMinutes && currentMinutes < iqamahMinutes) {
            return iqamahMinutes - currentMinutes;
        }
    }
    
    // Если мы находимся после всех икаматов или до первого азана
    return -1;
}

int PrayerSchedule::getSecondsToIqamah(uint8_t hour, uint8_t minute, uint8_t second, uint8_t dayOfWeek) const {
    if (!scheduleLoaded) {
        return -1;
    }
    
    // Пятница - день недели 5 (0=воскресенье, 1=понедельник, ..., 5=пятница, 6=суббота)
    // В пятницу не показываем время до икамата для джума (обеденного намаза)
    bool isFriday = (dayOfWeek == 5);
    
    // Преобразуем текущее время в секунды
    int currentSeconds = hour * 3600 + minute * 60 + second;
    
    // Ищем ближайший икамат, но только ПОСЛЕ азана
    for (int i = 0; i < 6; i++) {
        PrayerTime prayer = currentSchedule.prayers[i];
        int prayerSeconds = prayer.hour * 3600 + prayer.minute * 60;
        
        // Если это пятница и обеденный намаз (Зухр), пропускаем
        if (isFriday && prayer.name == "Зухр") {
            continue;
        }
        
        // Получаем время икамата для этой молитвы
        PrayerTime iqamah = getIqamahTime(i);
        int iqamahSeconds = iqamah.hour * 3600 + iqamah.minute * 60;
        
        // Отчет времени до икамата начинается только ПОСЛЕ азана
        // Текущее время должно быть >= времени азана и < времени икамата
        if (currentSeconds >= prayerSeconds && currentSeconds < iqamahSeconds) {
            return iqamahSeconds - currentSeconds;
        }
    }
    
    // Если мы находимся после всех икаматов или до первого азана
    return -1;
}

bool PrayerSchedule::isJumaPrayer(const String& prayerName, uint8_t dayOfWeek) const {
    // Пятница - день недели 5
    return (dayOfWeek == 5 && prayerName == "Зухр");
}

int PrayerSchedule::getCurrentPrayerIndex(uint8_t hour, uint8_t minute) const {
    if (!scheduleLoaded) {
        return -1;
    }
    
    // Проверяем каждую молитву на совпадение времени
    for (int i = 0; i < 6; i++) {
        if (currentSchedule.prayers[i].hour == hour && 
            currentSchedule.prayers[i].minute == minute) {
            return i;  // Возвращаем индекс молитвы (0-5)
        }
    }
    
    return -1;  // Время молитвы не совпадает
}
