#include <Arduino.h>
#include <board.h>
#include <rtc.h>
#include <prayer_schedule.h>
#include <wifi_ap.h>
#include <seven_segment.h>
#include <top_board.h>
#include <display_timer.h>

// Глобальный объект для управления расписанием молитв
PrayerSchedule prayerSchedule;

void setup() {
  // Сначала инициализируем Serial для отладки
  Serial.begin(9600);
  
  // Ждем подключения Serial монитора (до 5 секунд)
  // Это позволяет увидеть первые сообщения после reset
  unsigned long startTime = millis();
  while (!Serial && millis() - startTime < 5000) {
    delay(10);
  }
  
  
  // Инициализация пинов и интерфейсов платы
  boardInit();
  
  // Инициализация драйвера семисегментного индикатора
  sevenSegmentInit();
  
  // Инициализация данных отображения для мультиплексирования
  initDisplayData();
  
  // Инициализация библиотеки верхней платы
  topBoardInit();
  initTopBoardData();
  
  // Инициализация RTC
  rtcInit();
  
  // Получаем актуальное время из RTC и устанавливаем данные для верхней платы
  DateTime now = rtcGetTime();
  HijriDate hijri = rtcGetHijriDate();
  
  // Преобразуем день недели из RTClib (0=воскресенье) в нашу систему (0=суббота)
  // RTClib: 0=воскресенье, 1=понедельник, ..., 6=суббота
  // Наша система: 0=суббота, 1=воскресенье, ..., 6=пятница
  uint8_t rtcDayOfWeek = now.dayOfTheWeek(); // 0=воскресенье
  uint8_t ourDayOfWeek = (rtcDayOfWeek + 1) % 7; // Преобразуем: 0->1, 1->2, ..., 6->0
  
  TopBoardData topData;
  topData.year = now.year();
  topData.month = now.month();
  topData.day = now.day();
  topData.hours = now.hour();
  topData.minutes = now.minute();
  topData.seconds = now.second();
  topData.weekday = getWeekdayCode(ourDayOfWeek);
  topData.hijriMonth = getHijriMonthCode(hijri.month);
  topData.calendarType = CALENDAR_GREGORIAN;
  topData.colonEnabled = true;
  setTopBoardData(topData);
  
  // Инициализация файловой системы и загрузка расписания молитв
  if (prayerSchedule.begin()) {
    // Загружаем расписание на текущую дату
    prayerSchedule.loadCurrentSchedule();
  }
  
  // Инициализация точки доступа WiFi
  wifiApInit();
  
  // Инициализация таймера для мультиплексирования
  // Это обеспечивает стабильную индикацию независимо от работы WiFi
  displayTimerInit();
  displayTimerStart();
}

// Буфер для чтения serial команд
#define SERIAL_BUFFER_SIZE 128
char serialBuffer[SERIAL_BUFFER_SIZE];
int serialBufferIndex = 0;

// Обработка serial команд
void processSerialCommand(const char* command) {
  // Пропускаем пробелы в начале
  while (*command == ' ') command++;
  
  // Команда SETTIME YYYY-MM-DD HH:MM:SS
  if (strncmp(command, "SETTIME ", 8) == 0) {
    const char* timeStr = command + 8;
    if (rtcSetTimeFromString(timeStr)) {
      Serial.println("Время установлено успешно!");
      rtcPrintTime();
    }
  }
  // Команда SETTIME DD.MM.YYYY HH:MM:SS (российский формат)
  else if (strncmp(command, "SETTIME_RU ", 11) == 0) {
    const char* timeStr = command + 11;
    if (rtcSetTimeFromRussianFormat(timeStr)) {
      Serial.println("Время установлено успешно!");
      rtcPrintTime();
    }
  }
  // Команда TIME HH:MM:SS (установить только время)
  else if (strncmp(command, "TIME ", 5) == 0) {
    const char* timeStr = command + 5;
    int hour, minute, second;
    if (sscanf(timeStr, "%d:%d:%d", &hour, &minute, &second) == 3) {
      DateTime now = rtcGetTime();
      DateTime newTime(now.year(), now.month(), now.day(), hour, minute, second);
      rtcSetTime(newTime);
      Serial.println("Время установлено успешно!");
      rtcPrintTime();
    } else {
      Serial.println("Ошибка! Ожидается формат: TIME HH:MM:SS");
    }
  }
  // Команда DATE DD.MM.YYYY (установить только дату)
  else if (strncmp(command, "DATE ", 5) == 0) {
    const char* dateStr = command + 5;
    int day, month, year;
    if (sscanf(dateStr, "%d.%d.%d", &day, &month, &year) == 3) {
      DateTime now = rtcGetTime();
      DateTime newDate(year, month, day, now.hour(), now.minute(), now.second());
      rtcSetTime(newDate);
      Serial.println("Дата установлена успешно!");
      rtcPrintTime();
    } else {
      Serial.println("Ошибка! Ожидается формат: DATE DD.MM.YYYY");
    }
  }
  // Команда HELP - показать справку
  else if (strcmp(command, "HELP") == 0) {
    Serial.println("\n=== Доступные команды ===");
    Serial.println("SETTIME YYYY-MM-DD HH:MM:SS  - Установить дату и время");
    Serial.println("SETTIME_RU DD.MM.YYYY HH:MM:SS - Установить дату и время (российский формат)");
    Serial.println("TIME HH:MM:SS                - Установить только время");
    Serial.println("DATE DD.MM.YYYY              - Установить только дату");
    Serial.println("HELP                         - Показать эту справку");
    Serial.println("============================\n");
  }
  else if (strlen(command) > 0) {
    Serial.print("Неизвестная команда: ");
    Serial.println(command);
    Serial.println("Введите HELP для списка доступных команд");
  }
  displayToIqamah(BOARD_BOTTOM, 0, false);
}

void loop() {
  // Обработка serial команд
  while (Serial.available()) {
    char c = Serial.read();
    
    // Если получен перевод строки или возврат каретки
    if (c == '\n' || c == '\r') {
      // Завершаем строку и обрабатываем команду
      serialBuffer[serialBufferIndex] = '\0';
      if (serialBufferIndex > 0) {
        processSerialCommand(serialBuffer);
      }
      serialBufferIndex = 0;
    } 
    // Если есть место в буфере
    else if (serialBufferIndex < SERIAL_BUFFER_SIZE - 1) {
      serialBuffer[serialBufferIndex++] = c;
    }
  }
  
  // Обработка запросов веб-сервера
  wifiApHandleClient();
  
  // Каждую секунду обновляем данные отображения
  static unsigned long lastUpdateTime = 0;
  static uint8_t lastHour = 255;
  static uint8_t lastMinute = 255;
  static int lastMinutesToIqamah = -2;
  static bool scheduleDisplayed = false;
  
  // Состояние для 7-й строки (индикация до икамата)
  static enum {
    IQAMAH_STATE_BEFORE_ADHAN,    // До азана - пусто
    IQAMAH_STATE_MINUTES,         // После азана - отображаем минуты
    IQAMAH_STATE_SECONDS,         // Менее 1 минуты - отображаем секунды с миганием
    IQAMAH_STATE_POST_IQAMAH     // После икамата - моргаем еще одну минуту, потом пусто
  } iqamahState = IQAMAH_STATE_BEFORE_ADHAN;
  
  static unsigned long lastBlinkTime = 0;
  static bool blinkState = false;
  static unsigned long postIqamahStartTime = 0;
  
  // Состояние для моргания времени молитвы
  static enum {
    PRAYER_BLINK_IDLE,            // Нет активного моргания
    PRAYER_BLINK_ACTIVE           // Моргаем время молитвы
  } prayerBlinkState = PRAYER_BLINK_IDLE;
  
  static int blinkingPrayerIndex = -1;  // Индекс моргающей молитвы (0-5)
  static unsigned long prayerBlinkStartTime = 0;  // Время начала моргания
  
  // Переключение между календарями каждые 5 секунд
  static unsigned long lastCalendarSwitchTime = 0;
  static CalendarType currentCalendarType = CALENDAR_GREGORIAN;
  static HijriDate currentHijriDate;
  
  unsigned long currentTime = millis();
  
  // Переключение типа календаря каждые 5 секунд
  if (currentTime - lastCalendarSwitchTime >= 5000) {
    lastCalendarSwitchTime = currentTime;
    
    // Переключаем между григорианским и хиджрским календарями
    if (currentCalendarType == CALENDAR_GREGORIAN) {
      currentCalendarType = CALENDAR_HIJRI;
      // Получаем дату хиджры
      currentHijriDate = rtcGetHijriDate();
    } else {
      currentCalendarType = CALENDAR_GREGORIAN;
    }
  }
  
    // Обновление данных (каждую секунду)
    if (currentTime - lastUpdateTime >= 1000) {
      lastUpdateTime = currentTime;
      
      // Получаем текущее время
      DateTime now = rtcGetTime();
      
      // Обновляем данные на верхней плате
      HijriDate hijri = rtcGetHijriDate();
      
      // Преобразуем день недели из RTClib (0=воскресенье) в нашу систему (0=суббота)
      uint8_t rtcDayOfWeek = now.dayOfTheWeek();
      uint8_t ourDayOfWeek = (rtcDayOfWeek + 1) % 7;
      
      TopBoardData topData;
      
      // В зависимости от типа календаря устанавливаем соответствующие данные
      if (currentCalendarType == CALENDAR_GREGORIAN) {
        // Григорианский календарь
        topData.year = now.year();
        topData.month = now.month();
        topData.day = now.day();
      } else {
        // Хиджра календарь
        topData.year = currentHijriDate.year;
        topData.month = currentHijriDate.month;
        topData.day = currentHijriDate.day;
      }
      
      topData.hours = now.hour();
      topData.minutes = now.minute();
      topData.seconds = now.second();
      topData.weekday = getWeekdayCode(ourDayOfWeek);
      topData.hijriMonth = getHijriMonthCode(hijri.month);
      topData.calendarType = currentCalendarType;
      topData.colonEnabled = true;
      setTopBoardData(topData);
      
      // Проверяем время следующей молитвы и икамата
      if (prayerSchedule.isLoaded()) {
        // Получаем время до икамата
        int minutesToIqamah = prayerSchedule.getMinutesToIqamah(now.hour(), now.minute(), now.dayOfTheWeek());
        
        // Проверяем, наступило ли время какой-либо молитвы
        int currentPrayerIndex = prayerSchedule.getCurrentPrayerIndex(now.hour(), now.minute());
        
        // Если время молитвы наступило и нет активного моргания, начинаем моргать
        if (currentPrayerIndex != -1 && prayerBlinkState == PRAYER_BLINK_IDLE) {
          prayerBlinkState = PRAYER_BLINK_ACTIVE;
          blinkingPrayerIndex = currentPrayerIndex;
          prayerBlinkStartTime = currentTime;
          lastBlinkTime = currentTime;
          blinkState = true;
        }
        
        // Выводим информацию каждую минуту или когда изменилось время до икамата
        if (now.hour() != lastHour || now.minute() != lastMinute || minutesToIqamah != lastMinutesToIqamah) {
          lastHour = now.hour();
          lastMinute = now.minute();
          lastMinutesToIqamah = minutesToIqamah;
          
          // Отображаем расписание молитв на индикаторах (строки 1-6)
          const DailySchedule& schedule = prayerSchedule.getCurrentSchedule();
          
          // Если нет активного моргания молитвы, отображаем все молитвы
          if (prayerBlinkState == PRAYER_BLINK_IDLE) {
            // Отображаем все молитвы на соответствующих строках (1-6)
            // Строка 1 - Фаджр (утренний намаз)
            // Строка 2 - Восход
            // Строка 3 - Зухр (обеденный намаз)
            // Строка 4 - Аср (послеобеденный намаз)
            // Строка 5 - Магриб (вечерний намаз)
            // Строка 6 - Иша (ночной намаз)
            displayAllPrayers(BOARD_BOTTOM, schedule);
          } else {
            // Если есть активное моргание, отображаем только ту молитву, которая моргает
            for (int i = 0; i < 6; i++) {
              if (i == blinkingPrayerIndex) {
                // Моргающую молитву обрабатываем в секции мигания
                clearRowData(i + 1);
              } else {
                // Отображаем остальные молитвы нормально
                PrayerTime prayer = schedule.prayers[i];
                setRowData(i + 1, prayer.hour, prayer.minute, true);
              }
            }
          }
          
          // Логика отображения 7-й строки (индикация до икамата)
          if (minutesToIqamah < 0) {
            // До азана - пусто
            if (iqamahState != IQAMAH_STATE_SECONDS && iqamahState != IQAMAH_STATE_POST_IQAMAH) {
              iqamahState = IQAMAH_STATE_BEFORE_ADHAN;
              clearRow7(BOARD_BOTTOM);
            }
          } else if (minutesToIqamah >= 2) {
            // После азана, более 1 минуты - отображаем минуты
            iqamahState = IQAMAH_STATE_MINUTES;
            displayToIqamah(BOARD_BOTTOM, minutesToIqamah, true);
          } else if (minutesToIqamah == 1) {
            // Последняя минута (1 минута) - отображаем секунды с миганием
            if (iqamahState != IQAMAH_STATE_SECONDS && iqamahState != IQAMAH_STATE_POST_IQAMAH) {
              iqamahState = IQAMAH_STATE_SECONDS;
              lastBlinkTime = currentTime;
            }
          }
          
          // // Если minutesToIqamah изменился с 1 на -1, переходим в состояние секунд

          
        }
      }
    }
  
  // Обработка мигания для 7-й строки и молитвы (каждые 500 мс)
  if (currentTime - lastBlinkTime >= 500) {
    lastBlinkTime = currentTime;
    blinkState = !blinkState;
    
    // Обработка моргания молитвы
    if (prayerBlinkState == PRAYER_BLINK_ACTIVE) {
      // Проверяем, прошло ли 1 минута с начала моргания
      if (currentTime - prayerBlinkStartTime >= 60000) {
        // Прошла 1 минута, прекращаем моргание
        prayerBlinkState = PRAYER_BLINK_IDLE;
        blinkingPrayerIndex = -1;
        
        // Отображаем все молитвы нормально
        if (prayerSchedule.isLoaded()) {
          const DailySchedule& schedule = prayerSchedule.getCurrentSchedule();
          displayAllPrayers(BOARD_BOTTOM, schedule);
        }
      } else {
        // Продолжаем моргать
        if (blinkingPrayerIndex >= 0 && blinkingPrayerIndex < 6) {
          const DailySchedule& schedule = prayerSchedule.getCurrentSchedule();
          PrayerTime prayer = schedule.prayers[blinkingPrayerIndex];
          displayPrayerTimeBlink(BOARD_BOTTOM, blinkingPrayerIndex + 1, prayer.hour, prayer.minute, blinkState);
        }
      }
    }
    
    if (iqamahState == IQAMAH_STATE_SECONDS) {
      // Получаем текущее время и вычисляем секунды до икамата
      DateTime nowForSeconds = rtcGetTime();
      int secondsToIqamah = prayerSchedule.getSecondsToIqamah(
        nowForSeconds.hour(), 
        nowForSeconds.minute(), 
        nowForSeconds.second(), 
        nowForSeconds.dayOfTheWeek()
      );
      
      // Отображаем секунды с миганием (обратный отсчет)
      displayToIqamah(BOARD_BOTTOM, secondsToIqamah, !blinkState);
      
      // Если секунды <= 0, переходим в состояние после икамата
      if (secondsToIqamah <= 0) {
        iqamahState = IQAMAH_STATE_POST_IQAMAH;
        postIqamahStartTime = currentTime;
      }
    } else if (iqamahState == IQAMAH_STATE_POST_IQAMAH) {
      // Моргаем еще одну минуту после икамата
      // Моргаем "00" (нуль) каждую секунду
      displayToIqamah(BOARD_BOTTOM, 0, blinkState);
      
      // После одной минуты (60000 мс) отключаем индикацию
      if (currentTime - postIqamahStartTime >= 60000) {
        iqamahState = IQAMAH_STATE_BEFORE_ADHAN;
        clearRow7(BOARD_BOTTOM);
      }
    }
  }
}