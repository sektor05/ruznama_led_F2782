#include "wifi_ap.h"
#include "rtc.h"
#include "prayer_schedule.h"

// Объект веб-сервера
WebServer server(WEB_SERVER_PORT);

// Объект DNS сервера для Captive Portal
DNSServer dnsServer;

// Порт DNS сервера
const byte DNS_PORT = 53;

// Внешний объект расписания молитв (определен в main.cpp)
extern PrayerSchedule prayerSchedule;

// ============================================
// Глобальные переменные для обновления расписания
// ============================================
String ruznamaStatus = "idle";     // idle, updating, success, error
int ruznamaProgress = 0;           // Прогресс обновления (0-100)
String ruznamaErrorMessage = "";     // Сообщение об ошибке

// ============================================
// Глобальные переменные для обновления прошивки
// ============================================
String firmwareStatus = "idle";     // idle, updating, success, error
int firmwareProgress = 0;          // Прогресс обновления (0-100)
String firmwareErrorMessage = "";  // Сообщение об ошибке

// ============================================
// Предварительные объявления функций
// ============================================
String generateHTML();
String generateRuznamaUpdateHTML();
String generateFirmwareUpdateHTML();

// ============================================
// Генерация HTML страницы с временем и расписанием
// ============================================
String generateHTML() {
    String html = "<!DOCTYPE html>\n";
    html += "<html lang='ru'>\n";
    html += "<head>\n";
    html += "    <meta charset='UTF-8'>\n";
    html += "    <meta name='viewport' content='width=device-width, initial-scale=1.0'>\n";
    html += "    <title>Ruznama Table</title>\n";
    html += "    <style>\n";
    html += "        * {\n";
    html += "            margin: 0;\n";
    html += "            padding: 0;\n";
    html += "            box-sizing: border-box;\n";
    html += "        }\n";
    html += "        body {\n";
    html += "            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;\n";
    html += "            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);\n";
    html += "            min-height: 100vh;\n";
    html += "            display: flex;\n";
    html += "            flex-direction: column;\n";
    html += "            align-items: center;\n";
    html += "            justify-content: center;\n";
    html += "            padding: 20px;\n";
    html += "        }\n";
    html += "        .container {\n";
    html += "            background: rgba(255, 255, 255, 0.95);\n";
    html += "            border-radius: 20px;\n";
    html += "            box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);\n";
    html += "            padding: 40px;\n";
    html += "            max-width: 500px;\n";
    html += "            width: 100%;\n";
    html += "            text-align: center;\n";
    html += "        }\n";
    html += "        .time {\n";
    html += "            font-size: 72px;\n";
    html += "            font-weight: bold;\n";
    html += "            color: #333;\n";
    html += "            margin-bottom: 10px;\n";
    html += "            text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.1);\n";
    html += "        }\n";
    html += "        .date {\n";
    html += "            font-size: 24px;\n";
    html += "            color: #666;\n";
    html += "            margin-bottom: 20px;\n";
    html += "        }\n";
    html += "        .hijri-date {\n";
    html += "            font-size: 20px;\n";
    html += "            color: #888;\n";
    html += "            margin-bottom: 30px;\n";
    html += "            display: flex;\n";
    html += "            align-items: center;\n";
    html += "            justify-content: center;\n";
    html += "            gap: 10px;\n";
    html += "        }\n";
    html += "        .hijri-controls {\n";
    html += "            display: flex;\n";
    html += "            align-items: center;\n";
    html += "            gap: 5px;\n";
    html += "        }\n";
    html += "        .hijri-btn {\n";
    html += "            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);\n";
    html += "            color: white;\n";
    html += "            border: none;\n";
    html += "            border-radius: 8px;\n";
    html += "            width: 32px;\n";
    html += "            height: 32px;\n";
    html += "            font-size: 18px;\n";
    html += "            font-weight: bold;\n";
    html += "            cursor: pointer;\n";
    html += "            transition: transform 0.2s, box-shadow 0.2s;\n";
    html += "            display: flex;\n";
    html += "            align-items: center;\n";
    html += "            justify-content: center;\n";
    html += "        }\n";
    html += "        .hijri-btn:hover {\n";
    html += "            transform: translateY(-2px);\n";
    html += "            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);\n";
    html += "        }\n";
    html += "        .hijri-btn:active {\n";
    html += "            transform: translateY(0);\n";
    html += "        }\n";
    html += "        .update-btn {\n";
    html += "            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);\n";
    html += "            color: white;\n";
    html += "            border: none;\n";
    html += "            border-radius: 8px;\n";
    html += "            padding: 8px 16px;\n";
    html += "            font-size: 14px;\n";
    html += "            font-weight: 600;\n";
    html += "            cursor: pointer;\n";
    html += "            transition: transform 0.2s, box-shadow 0.2s;\n";
    html += "            margin-left: 10px;\n";
    html += "        }\n";
    html += "        .update-btn:hover {\n";
    html += "            transform: translateY(-2px);\n";
    html += "            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);\n";
    html += "        }\n";
    html += "        .update-btn:active {\n";
    html += "            transform: translateY(0);\n";
    html += "        }\n";
    html += "        .time-container {\n";
    html += "            display: flex;\n";
    html += "            align-items: center;\n";
    html += "            justify-content: center;\n";
    html += "            gap: 15px;\n";
    html += "            margin-bottom: 10px;\n";
    html += "        }\n";
    html += "        .schedule-title {\n";
    html += "            font-size: 28px;\n";
    html += "            color: #667eea;\n";
    html += "            margin-bottom: 20px;\n";
    html += "            font-weight: 600;\n";
    html += "        }\n";
    html += "        .schedule {\n";
    html += "            display: flex;\n";
    html += "            flex-direction: column;\n";
    html += "            gap: 12px;\n";
    html += "        }\n";
    html += "        .prayer {\n";
    html += "            display: flex;\n";
    html += "            justify-content: space-between;\n";
    html += "            align-items: center;\n";
    html += "            padding: 15px 20px;\n";
    html += "            background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);\n";
    html += "            border-radius: 12px;\n";
    html += "            transition: transform 0.2s, box-shadow 0.2s;\n";
    html += "        }\n";
    html += "        .prayer:hover {\n";
    html += "            transform: translateY(-2px);\n";
    html += "            box-shadow: 0 5px 15px rgba(0, 0, 0, 0.1);\n";
    html += "        }\n";
    html += "        .prayer-name {\n";
    html += "            font-size: 18px;\n";
    html += "            color: #333;\n";
    html += "            font-weight: 500;\n";
    html += "        }\n";
    html += "        .prayer-time {\n";
    html += "            font-size: 20px;\n";
    html += "            color: #667eea;\n";
    html += "            font-weight: bold;\n";
    html += "        }\n";
    html += "        .next-prayer {\n";
    html += "            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%) !important;\n";
    html += "        }\n";
    html += "        .next-prayer .prayer-name,\n";
    html += "        .next-prayer .prayer-time {\n";
    html += "            color: white;\n";
    html += "        }\n";
    html += "        .footer {\n";
    html += "            margin-top: 30px;\n";
    html += "            font-size: 14px;\n";
    html += "            color: #999;\n";
    html += "        }\n";
    html += "        @media (max-width: 480px) {\n";
    html += "            .time {\n";
    html += "                font-size: 56px;\n";
    html += "            }\n";
    html += "            .container {\n";
    html += "                padding: 30px 20px;\n";
    html += "            }\n";
    html += "            .hijri-date {\n";
    html += "                flex-direction: column;\n";
    html += "                gap: 8px;\n";
    html += "            }\n";
    html += "            .time-container {\n";
    html += "                gap: 10px;\n";
    html += "            }\n";
    html += "            .update-btn {\n";
    html += "                padding: 6px 12px;\n";
    html += "                font-size: 12px;\n";
    html += "            }\n";
    html += "            .hijri-btn {\n";
    html += "                width: 28px;\n";
    html += "                height: 28px;\n";
    html += "                font-size: 16px;\n";
    html += "            }\n";
    html += "        }\n";
    html += "    </style>\n";
    html += "    <script>\n";
    html += "        let hijriOffset = 0;\n";
    html += "        \n";
    html += "        // Автоматическое обновление времени каждую секунду\n";
    html += "        setInterval(function() {\n";
    html += "            fetch('/time')\n";
    html += "                .then(response => response.json())\n";
    html += "                .then(data => {\n";
    html += "                    document.getElementById('time').textContent = data.time;\n";
    html += "                    document.getElementById('date').textContent = data.date;\n";
    html += "                    document.getElementById('hijri').textContent = data.hijri.day + '.' + data.hijri.month + '.' + data.hijri.year + ' AH';\n";
    html += "                    hijriOffset = data.hijriOffset;\n";
    html += "                })\n";
    html += "                .catch(err => console.log('Ошибка обновления:', err));\n";
    html += "        }, 1000);\n";
    html += "        \n";
    html += "        // Функция для обновления времени с телефона\n";
    html += "        function updateTimeFromPhone() {\n";
    html += "            const now = new Date();\n";
    html += "            const timeData = {\n";
    html += "                year: now.getFullYear(),\n";
    html += "                month: now.getMonth() + 1,\n";
    html += "                day: now.getDate(),\n";
    html += "                hour: now.getHours(),\n";
    html += "                minute: now.getMinutes(),\n";
    html += "                second: now.getSeconds()\n";
    html += "            };\n";
    html += "            \n";
    html += "            fetch('/settime', {\n";
    html += "                method: 'POST',\n";
    html += "                headers: {\n";
    html += "                    'Content-Type': 'application/json'\n";
    html += "                },\n";
    html += "                body: JSON.stringify(timeData)\n";
    html += "            })\n";
    html += "            .then(response => response.json())\n";
    html += "            .then(data => {\n";
    html += "                if (data.success) {\n";
    html += "                    alert('Время успешно обновлено!');\n";
    html += "                } else {\n";
    html += "                    alert('Ошибка: ' + data.message);\n";
    html += "                }\n";
    html += "            })\n";
    html += "            .catch(err => {\n";
    html += "                console.error('Ошибка:', err);\n";
    html += "                alert('Ошибка соединения');\n";
    html += "            });\n";
    html += "        }\n";
    html += "        \n";
    html += "        // Функция для корректировки даты хиджры\n";
    html += "        function adjustHijriDate(delta) {\n";
    html += "            const newOffset = hijriOffset + delta;\n";
    html += "            \n";
    html += "            fetch('/hijri-offset', {\n";
    html += "                method: 'POST',\n";
    html += "                headers: {\n";
    html += "                    'Content-Type': 'application/json'\n";
    html += "                },\n";
    html += "                body: JSON.stringify({ offset: newOffset })\n";
    html += "            })\n";
    html += "            .then(response => response.json())\n";
    html += "            .then(data => {\n";
    html += "                if (data.success) {\n";
    html += "                    hijriOffset = data.offset;\n";
    html += "                    document.getElementById('hijri').textContent = data.hijri.day + '.' + data.hijri.month + '.' + data.hijri.year + ' AH';\n";
    html += "                } else {\n";
    html += "                    alert('Ошибка: ' + data.message);\n";
    html += "                }\n";
    html += "            })\n";
    html += "            .catch(err => {\n";
    html += "                console.error('Ошибка:', err);\n";
    html += "                alert('Ошибка соединения');\n";
    html += "            });\n";
    html += "        }\n";
    html += "    </script>\n";
    html += "</head>\n";
    html += "<body>\n";
    html += "    <div class='container'>\n";
    
    // Получаем текущее время
    DateTime now = rtcGetTime();
    char timeStr[9];
    snprintf(timeStr, sizeof(timeStr), "%02u:%02u:%02u", now.hour(), now.minute(), now.second());
    
    char dateStr[20];
    snprintf(dateStr, sizeof(dateStr), "%02u.%02u.%04u", now.day(), now.month(), now.year());
    
    // Получаем дату хиджры
    HijriDate hijri = rtcGetHijriDate();
    char hijriStr[30];
    snprintf(hijriStr, sizeof(hijriStr), "%d.%d.%d AH", hijri.day, hijri.month, hijri.year);
    
    html += "        <div class='time-container'>\n";
    html += "            <div class='time' id='time'>" + String(timeStr) + "</div>\n";
    html += "            <button class='update-btn' onclick='updateTimeFromPhone()'>↻</button>\n";
    html += "        </div>\n";
    html += "        <div class='date' id='date'>" + String(dateStr) + "</div>\n";
    html += "        <div class='hijri-date'>\n";
    html += "            <span id='hijri'>" + String(hijriStr) + "</span>\n";
    html += "            <div class='hijri-controls'>\n";
    html += "                <button class='hijri-btn' onclick='adjustHijriDate(-1)'>−</button>\n";
    html += "                <button class='hijri-btn' onclick='adjustHijriDate(1)'>+</button>\n";
    html += "            </div>\n";
    html += "        </div>\n";
    html += "        <div class='schedule-title'>Расписание молитв</div>\n";
    html += "        <div class='schedule'>\n";
    
    // Добавляем расписание молитв
    if (prayerSchedule.isLoaded()) {
        const DailySchedule& schedule = prayerSchedule.getCurrentSchedule();
        PrayerTime nextPrayer = prayerSchedule.getNextPrayer(now.hour(), now.minute());
        
        for (int i = 0; i < 6; i++) {
            PrayerTime prayer = schedule.prayers[i];
            char prayerTimeStr[6];
            snprintf(prayerTimeStr, sizeof(prayerTimeStr), "%02u:%02u", prayer.hour, prayer.minute);
            
            String prayerClass = "prayer";
            if (nextPrayer.name == prayer.name) {
                prayerClass += " next-prayer";
            }
            
            html += "            <div class='" + prayerClass + "'>\n";
            html += "                <div class='prayer-name'>" + prayer.name + "</div>\n";
            html += "                <div class='prayer-time'>" + String(prayerTimeStr) + "</div>\n";
            html += "            </div>\n";
        }
    } else {
        html += "            <div class='prayer'>\n";
        html += "                <div class='prayer-name'>Расписание не загружено</div>\n";
        html += "            </div>\n";
    }
    
    html += "        </div>\n";
    html += "        <div class='footer'>Ruznama Table ESP32-S2</div>\n";
    html += "    </div>\n";
    html += "</body>\n";
    html += "</html>\n";
    
    return html;
}

// ============================================
// Обработчик корневой страницы
// ============================================
void handleRoot() {
    Serial.println("Web: Запрос корневой страницы");
    String html = generateHTML();
    server.send(200, "text/html", html);
}

// ============================================
// Обработчик API для получения времени
// ============================================
void handleTimeAPI() {
    DateTime now = rtcGetTime();
    
    char timeStr[9];
    snprintf(timeStr, sizeof(timeStr), "%02u:%02u:%02u", now.hour(), now.minute(), now.second());
    
    char dateStr[20];
    snprintf(dateStr, sizeof(dateStr), "%02u.%02u.%04u", now.day(), now.month(), now.year());
    
    String json = "{\n";
    json += "  \"time\": \"" + String(timeStr) + "\",\n";
    json += "  \"date\": \"" + String(dateStr) + "\",\n";
    json += "  \"hijri\": " + rtcGetHijriDateJSON() + ",\n";
    json += "  \"hijriOffset\": " + String(rtcGetHijriOffset()) + "\n";
    json += "}";
    
    server.send(200, "application/json", json);
}

// ============================================
// Обработчик API для установки времени с телефона
// ============================================
void handleSetTimeAPI() {
    Serial.println("Web: Запрос на установку времени");
    
    if (server.hasArg("plain")) {
        String body = server.arg("plain");
        
        // Парсим JSON
        int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
        
        // Ищем значения в JSON
        int pos = body.indexOf("\"year\":");
        if (pos != -1) {
            year = body.substring(pos + 7).toInt();
        }
        
        pos = body.indexOf("\"month\":");
        if (pos != -1) {
            month = body.substring(pos + 8).toInt();
        }
        
        pos = body.indexOf("\"day\":");
        if (pos != -1) {
            day = body.substring(pos + 6).toInt();
        }
        
        pos = body.indexOf("\"hour\":");
        if (pos != -1) {
            hour = body.substring(pos + 7).toInt();
        }
        
        pos = body.indexOf("\"minute\":");
        if (pos != -1) {
            minute = body.substring(pos + 9).toInt();
        }
        
        pos = body.indexOf("\"second\":");
        if (pos != -1) {
            second = body.substring(pos + 9).toInt();
        }
        
        // Проверяем валидность данных
        if (year >= 2000 && year <= 2100 && month >= 1 && month <= 12 && 
            day >= 1 && day <= 31 && hour >= 0 && hour <= 23 && 
            minute >= 0 && minute <= 59 && second >= 0 && second <= 59) {
            
            DateTime dateTime(year, month, day, hour, minute, second);
            rtcSetTime(dateTime);
            
            String response = "{\n";
            response += "  \"success\": true,\n";
            response += "  \"message\": \"Время установлено успешно\"\n";
            response += "}";
            
            server.send(200, "application/json", response);
        } else {
            String response = "{\n";
            response += "  \"success\": false,\n";
            response += "  \"message\": \"Неверный формат времени\"\n";
            response += "}";
            
            server.send(400, "application/json", response);
        }
    } else {
        String response = "{\n";
        response += "  \"success\": false,\n";
        response += "  \"message\": \"Нет данных\"\n";
        response += "}";
        
        server.send(400, "application/json", response);
    }
}

// ============================================
// Обработчик API для корректировки даты хиджры
// ============================================
void handleHijriOffsetAPI() {
    Serial.println("Web: Запрос на корректировку даты хиджры");
    
    if (server.hasArg("plain")) {
        String body = server.arg("plain");
        
        // Парсим JSON
        int offset = 0;
        
        // Ищем значение offset в JSON
        int pos = body.indexOf("\"offset\":");
        if (pos != -1) {
            offset = body.substring(pos + 9).toInt();
        }
        
        // Ограничиваем диапазон смещения (-30 до +30 дней)
        if (offset < -30) offset = -30;
        if (offset > 30) offset = 30;
        
        // Сохраняем смещение
        rtcSetHijriOffset(offset);
        
        String response = "{\n";
        response += "  \"success\": true,\n";
        response += "  \"offset\": " + String(offset) + ",\n";
        response += "  \"hijri\": " + rtcGetHijriDateJSON() + "\n";
        response += "}";
        
        server.send(200, "application/json", response);
    } else {
        String response = "{\n";
        response += "  \"success\": false,\n";
        response += "  \"message\": \"Нет данных\"\n";
        response += "}";
        
        server.send(400, "application/json", response);
    }
}

// ============================================
// Обработчик для несуществующих страниц (Captive Portal)
// ============================================
void handleNotFound() {
    Serial.println("Web: Перенаправление на главную страницу (Captive Portal)");
    
    // Перенаправляем на корневую страницу
    server.sendHeader("Location", String("/"), true);
    server.send(302, "text/plain", "");
}

// ============================================
// Обработчик GET запроса для страницы обновления расписания
// ============================================
void handleRuznamaUpdateGET() {
    Serial.println("Web: Запрос страницы обновления расписания");
    String html = generateRuznamaUpdateHTML();
    server.send(200, "text/html", html);
}

// ============================================
// Обработчик POST запроса для загрузки расписания
// ============================================
void handleRuznamaUpdatePOST() {
    Serial.println("Web: Запрос на загрузку расписания");
    
    // Проверяем наличие файла
    HTTPUpload& upload = server.upload();
    
    static File uploadFile;
    
    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Ruznama: Начало загрузки файла: %s\n", upload.filename.c_str());
        Serial.printf("Ruznama: Размер файла: %u байт\n", upload.totalSize);
        
        // Проверяем расширение файла
        String filename = upload.filename;
        if (!filename.endsWith(".json")) {
            Serial.println("Ruznama: Неверное расширение файла");
            
            String response = "{\n";
            response += "  \"success\": false,\n";
            response += "  \"message\": \"Файл должен иметь расширение .json\"\n";
            response += "}";
            
            server.send(400, "application/json", response);
            return;
        }
        
        // Сбрасываем статус
        ruznamaStatus = "updating";
        ruznamaProgress = 0;
        ruznamaErrorMessage = "";
        
        // Открываем файл для записи
        uploadFile = LittleFS.open("/year_schedule.json", "w");
        
        if (!uploadFile) {
            Serial.println("Ruznama: Ошибка открытия файла для записи");
            
            String response = "{\n";
            response += "  \"success\": false,\n";
            response += "  \"message\": \"Ошибка открытия файла для записи\"\n";
            response += "}";
            
            ruznamaStatus = "error";
            ruznamaErrorMessage = "Ошибка открытия файла для записи";
            server.send(500, "application/json", response);
            return;
        }
        
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        // Записываем данные в файл по мере поступления
        if (uploadFile) {
            size_t written = uploadFile.write(upload.buf, upload.currentSize);
            ruznamaProgress = (upload.totalSize - upload.currentSize + written) * 100 / upload.totalSize;
            Serial.printf("Ruznama: Записано %u байт (%u%%)\n", written, ruznamaProgress);
        }
        
    } else if (upload.status == UPLOAD_FILE_END) {
        Serial.printf("Ruznama: Загрузка завершена. Размер: %u байт\n", upload.totalSize);
        
        // Закрываем файл
        if (uploadFile) {
            uploadFile.close();
        }
        
        // Перезагружаем расписание
        Serial.println("Ruznama: Перезагрузка расписания...");
        if (prayerSchedule.loadCurrentSchedule()) {
            Serial.println("Ruznama: Расписание успешно загружено");
            
            String response = "{\n";
            response += "  \"success\": true,\n";
            response += "  \"message\": \"Расписание успешно обновлено\"\n";
            response += "}";
            
            ruznamaStatus = "success";
            ruznamaProgress = 100;
            server.send(200, "application/json", response);
        } else {
            Serial.println("Ruznama: Ошибка загрузки расписания");
            
            String response = "{\n";
            response += "  \"success\": false,\n";
            response += "  \"message\": \"Ошибка загрузки расписания\"\n";
            response += "}";
            
            ruznamaStatus = "error";
            ruznamaErrorMessage = "Ошибка загрузки расписания";
            server.send(500, "application/json", response);
        }
    }
}

// ============================================
// Обработчик API для получения статуса обновления расписания
// ============================================
void handleRuznamaUpdateStatus() {
    String json = "{\n";
    json += "  \"status\": \"" + ruznamaStatus + "\",\n";
    json += "  \"percent\": " + String(ruznamaProgress);
    
    if (ruznamaStatus == "error" && ruznamaErrorMessage.length() > 0) {
        json += ",\n";
        json += "  \"message\": \"" + ruznamaErrorMessage + "\"";
    }
    
    json += "\n}";
    
    server.send(200, "application/json", json);
}

// ============================================
// Генерация HTML страницы для обновления прошивки
// ============================================
String generateFirmwareUpdateHTML() {
    String html = "<!DOCTYPE html>\n";
    html += "<html lang='ru'>\n";
    html += "<head>\n";
    html += "    <meta charset='UTF-8'>\n";
    html += "    <meta name='viewport' content='width=device-width, initial-scale=1.0'>\n";
    html += "    <title>Обновление прошивки - Ruznama Table</title>\n";
    html += "    <style>\n";
    html += "        * {\n";
    html += "            margin: 0;\n";
    html += "            padding: 0;\n";
    html += "            box-sizing: border-box;\n";
    html += "        }\n";
    html += "        body {\n";
    html += "            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;\n";
    html += "            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);\n";
    html += "            min-height: 100vh;\n";
    html += "            display: flex;\n";
    html += "            flex-direction: column;\n";
    html += "            align-items: center;\n";
    html += "            justify-content: center;\n";
    html += "            padding: 20px;\n";
    html += "        }\n";
    html += "        .container {\n";
    html += "            background: rgba(255, 255, 255, 0.95);\n";
    html += "            border-radius: 20px;\n";
    html += "            box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);\n";
    html += "            padding: 40px;\n";
    html += "            max-width: 500px;\n";
    html += "            width: 100%;\n";
    html += "            text-align: center;\n";
    html += "        }\n";
    html += "        h1 {\n";
    html += "            color: #667eea;\n";
    html += "            margin-bottom: 10px;\n";
    html += "            font-size: 28px;\n";
    html += "        }\n";
    html += "        .subtitle {\n";
    html += "            color: #666;\n";
    html += "            margin-bottom: 30px;\n";
    html += "            font-size: 14px;\n";
    html += "        }\n";
    html += "        .upload-area {\n";
    html += "            border: 2px dashed #667eea;\n";
    html += "            border-radius: 12px;\n";
    html += "            padding: 40px 20px;\n";
    html += "            margin-bottom: 20px;\n";
    html += "            cursor: pointer;\n";
    html += "            transition: all 0.3s;\n";
    html += "        }\n";
    html += "        .upload-area:hover {\n";
    html += "            background: rgba(102, 126, 234, 0.05);\n";
    html += "            border-color: #764ba2;\n";
    html += "        }\n";
    html += "        .upload-icon {\n";
    html += "            font-size: 48px;\n";
    html += "            color: #667eea;\n";
    html += "            margin-bottom: 15px;\n";
    html += "        }\n";
    html += "        .upload-text {\n";
    html += "            color: #333;\n";
    html += "            font-size: 16px;\n";
    html += "            margin-bottom: 5px;\n";
    html += "        }\n";
    html += "        .upload-hint {\n";
    html += "            color: #999;\n";
    html += "            font-size: 12px;\n";
    html += "        }\n";
    html += "        input[type='file'] {\n";
    html += "            display: none;\n";
    html += "        }\n";
    html += "        .btn {\n";
    html += "            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);\n";
    html += "            color: white;\n";
    html += "            border: none;\n";
    html += "            border-radius: 8px;\n";
    html += "            padding: 12px 30px;\n";
    html += "            font-size: 16px;\n";
    html += "            font-weight: 600;\n";
    html += "            cursor: pointer;\n";
    html += "            transition: transform 0.2s, box-shadow 0.2s;\n";
    html += "            width: 100%;\n";
    html += "        }\n";
    html += "        .btn:hover {\n";
    html += "            transform: translateY(-2px);\n";
    html += "            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);\n";
    html += "        }\n";
    html += "        .btn:disabled {\n";
    html += "            background: #ccc;\n";
    html += "            cursor: not-allowed;\n";
    html += "            transform: none;\n";
    html += "            box-shadow: none;\n";
    html += "        }\n";
    html += "        .progress-container {\n";
    html += "            margin-top: 20px;\n";
    html += "            display: none;\n";
    html += "        }\n";
    html += "        .progress-bar {\n";
    html += "            background: #e0e0e0;\n";
    html += "            border-radius: 10px;\n";
    html += "            height: 20px;\n";
    html += "            overflow: hidden;\n";
    html += "        }\n";
    html += "        .progress-fill {\n";
    html += "            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);\n";
    html += "            height: 100%;\n";
    html += "            width: 0%;\n";
    html += "            transition: width 0.3s;\n";
    html += "        }\n";
    html += "        .progress-text {\n";
    html += "            margin-top: 10px;\n";
    html += "            color: #666;\n";
    html += "            font-size: 14px;\n";
    html += "        }\n";
    html += "        .status-message {\n";
    html += "            margin-top: 15px;\n";
    html += "            padding: 10px;\n";
    html += "            border-radius: 8px;\n";
    html += "            font-size: 14px;\n";
    html += "            display: none;\n";
    html += "        }\n";
    html += "        .status-success {\n";
    html += "            background: #d4edda;\n";
    html += "            color: #155724;\n";
    html += "        }\n";
    html += "        .status-error {\n";
    html += "            background: #f8d7da;\n";
    html += "            color: #721c24;\n";
    html += "        }\n";
    html += "        .status-updating {\n";
    html += "            background: #d1ecf1;\n";
    html += "            color: #0c5460;\n";
    html += "        }\n";
    html += "        .warning {\n";
    html += "            margin-top: 20px;\n";
    html += "            padding: 15px;\n";
    html += "            background: #fff3cd;\n";
    html += "            border-left: 4px solid #ffc107;\n";
    html += "            border-radius: 8px;\n";
    html += "            text-align: left;\n";
    html += "        }\n";
    html += "        .warning-title {\n";
    html += "            font-weight: bold;\n";
    html += "            color: #856404;\n";
    html += "            margin-bottom: 5px;\n";
    html += "        }\n";
    html += "        .warning-text {\n";
    html += "            color: #856404;\n";
    html += "            font-size: 13px;\n";
    html += "            line-height: 1.5;\n";
    html += "        }\n";
    html += "        .back-link {\n";
    html += "            margin-top: 20px;\n";
    html += "            color: #667eea;\n";
    html += "            text-decoration: none;\n";
    html += "            font-size: 14px;\n";
    html += "        }\n";
    html += "        .back-link:hover {\n";
    html += "            text-decoration: underline;\n";
    html += "        }\n";
    html += "        @media (max-width: 480px) {\n";
    html += "            .container {\n";
    html += "                padding: 30px 20px;\n";
    html += "            }\n";
    html += "            h1 {\n";
    html += "                font-size: 24px;\n";
    html += "            }\n";
    html += "        }\n";
    html += "    </style>\n";
    html += "    <script>\n";
    html += "        let selectedFile = null;\n";
    html += "        let updateInProgress = false;\n";
    html += "        \n";
    html += "        // Ждём загрузки DOM перед добавлением обработчиков\n";
    html += "        document.addEventListener('DOMContentLoaded', function() {\n";
    html += "            // Обработка клика на область загрузки\n";
    html += "            document.querySelector('.upload-area').addEventListener('click', function() {\n";
    html += "                if (!updateInProgress) {\n";
    html += "                    document.getElementById('firmwareFile').click();\n";
    html += "                }\n";
    html += "            });\n";
    html += "            \n";
    html += "            // Обработка выбора файла\n";
    html += "            document.getElementById('firmwareFile').addEventListener('change', function(e) {\n";
    html += "                if (e.target.files.length > 0) {\n";
    html += "                    selectedFile = e.target.files[0];\n";
    html += "                    document.querySelector('.upload-text').textContent = selectedFile.name;\n";
    html += "                    document.querySelector('.upload-hint').textContent = 'Размер: ' + formatFileSize(selectedFile.size);\n";
    html += "                    document.getElementById('uploadBtn').disabled = false;\n";
    html += "                }\n";
    html += "            });\n";
    html += "            \n";
    html += "            // Загрузка файла\n";
    html += "            document.getElementById('uploadBtn').addEventListener('click', function() {\n";
    html += "                if (selectedFile && !updateInProgress) {\n";
    html += "                    uploadFirmware();\n";
    html += "                }\n";
    html += "            });\n";
    html += "        });\n";
    html += "        \n";
    html += "        // Форматирование размера файла\n";
    html += "        function formatFileSize(bytes) {\n";
    html += "            if (bytes === 0) return '0 Bytes';\n";
    html += "            const k = 1024;\n";
    html += "            const sizes = ['Bytes', 'KB', 'MB', 'GB'];\n";
    html += "            const i = Math.floor(Math.log(bytes) / Math.log(k));\n";
    html += "            return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];\n";
    html += "        }\n";
    html += "        \n";
        html += "        function uploadFirmware() {\n";
    html += "            updateInProgress = true;\n";
    html += "            document.getElementById('uploadBtn').disabled = true;\n";
    html += "            document.getElementById('uploadBtn').textContent = 'Загрузка...';\n";
    html += "            document.querySelector('.progress-container').style.display = 'block';\n";
    html += "            showStatus('updating', 'Обновление прошивки...');\n";
    html += "            \n";
    html += "            const formData = new FormData();\n";
    html += "            formData.append('firmware', selectedFile);\n";
    html += "            \n";
    html += "            fetch('/firmwareupdate', {\n";
    html += "                method: 'POST',\n";
    html += "                body: formData\n";
    html += "            })\n";
    html += "            .then(response => response.json())\n";
    html += "            .then(data => {\n";
    html += "                if (data.success) {\n";
    html += "                    showStatus('success', data.message);\n";
    html += "                    document.querySelector('.progress-fill').style.width = '100%';\n";
    html += "                    document.querySelector('.progress-text').textContent = '100%';\n";
    html += "                    \n";
    html += "                    // Устройство перезагрузится\n";
    html += "                    setTimeout(function() {\n";
    html += "                        showStatus('updating', 'Устройство перезагружается...');\n";
    html += "                    }, 2000);\n";
    html += "                } else {\n";
    html += "                    showStatus('error', data.message);\n";
    html += "                    resetUpload();\n";
    html += "                }\n";
    html += "            })\n";
    html += "            .catch(err => {\n";
    html += "                console.error('Ошибка:', err);\n";
    html += "                showStatus('error', 'Ошибка соединения');\n";
    html += "                resetUpload();\n";
    html += "            });\n";
    html += "            \n";
    html += "            // Отслеживание прогресса\n";
    html += "            const progressInterval = setInterval(function() {\n";
    html += "                fetch('/firmwareupdate/status')\n";
    html += "                    .then(response => response.json())\n";
    html += "                    .then(data => {\n";
    html += "                        document.querySelector('.progress-fill').style.width = data.percent + '%';\n";
    html += "                        document.querySelector('.progress-text').textContent = data.percent + '%';\n";
    html += "                        \n";
    html += "                        if (data.status === 'error') {\n";
    html += "                            showStatus('error', data.message || 'Ошибка обновления');\n";
    html += "                            clearInterval(progressInterval);\n";
    html += "                            resetUpload();\n";
    html += "                        } else if (data.status === 'success') {\n";
    html += "                            clearInterval(progressInterval);\n";
    html += "                        }\n";
    html += "                    })\n";
    html += "                    .catch(err => console.log('Ошибка получения статуса:', err));\n";
    html += "            }, 500);\n";
    html += "        }\n";
    html += "        \n";
    html += "        function showStatus(type, message) {\n";
    html += "            const statusEl = document.querySelector('.status-message');\n";
    html += "            statusEl.className = 'status-message status-' + type;\n";
    html += "            statusEl.textContent = message;\n";
    html += "            statusEl.style.display = 'block';\n";
    html += "        }\n";
    html += "        \n";
    html += "        function resetUpload() {\n";
    html += "            updateInProgress = false;\n";
    html += "            document.getElementById('uploadBtn').disabled = true;\n";
    html += "            document.getElementById('uploadBtn').textContent = 'Загрузить прошивку';\n";
    html += "            selectedFile = null;\n";
    html += "            document.querySelector('.upload-text').textContent = 'Нажмите для выбора файла';\n";
    html += "            document.querySelector('.upload-hint').textContent = 'Поддерживаются файлы .bin';\n";
    html += "            document.getElementById('firmwareFile').value = '';\n";
    html += "        }\n";
    html += "    </script>\n";
    html += "</head>\n";
    html += "<body>\n";
    html += "    <div class='container'>\n";
    html += "        <h1>Обновление прошивки</h1>\n";
    html += "        <div class='subtitle'>Ruznama Table ESP32-S2</div>\n";
    html += "        \n";
    html += "        <div class='upload-area'>\n";
    html += "            <div class='upload-icon'>📁</div>\n";
    html += "            <div class='upload-text'>Нажмите для выбора файла</div>\n";
    html += "            <div class='upload-hint'>Поддерживаются файлы .bin</div>\n";
    html += "            <input type='file' id='firmwareFile' accept='.bin'>\n";
    html += "        </div>\n";
    html += "        \n";
    html += "        <button class='btn' id='uploadBtn' disabled>Загрузить прошивку</button>\n";
    html += "        \n";
    html += "        <div class='progress-container'>\n";
    html += "            <div class='progress-bar'>\n";
    html += "                <div class='progress-fill'></div>\n";
    html += "            </div>\n";
    html += "            <div class='progress-text'>0%</div>\n";
    html += "        </div>\n";
    html += "        \n";
    html += "        <div class='status-message'></div>\n";
    html += "        \n";
    html += "        <div class='warning'>\n";
    html += "            <div class='warning-title'>⚠️ Внимание</div>\n";
    html += "            <div class='warning-text'>\n";
    html += "                • Не отключайте питание во время обновления<br>\n";
    html += "                • Устройство автоматически перезагрузится после завершения<br>\n";
    html += "                • Процесс может занять несколько минут<br>\n";
    html += "                • После перезагрузки подключитесь к сети снова\n";
    html += "            </div>\n";
    html += "        </div>\n";
    html += "        \n";
    html += "        <a href='/' class='back-link'>← Вернуться на главную</a>\n";
    html += "    </div>\n";
    html += "</body>\n";
    html += "</html>\n";
    
    return html;
}

// ============================================
// Генерация HTML страницы для обновления расписания рузнама
// ============================================
String generateRuznamaUpdateHTML() {
    String html = "<!DOCTYPE html>\n";
    html += "<html lang='ru'>\n";
    html += "<head>\n";
    html += "    <meta charset='UTF-8'>\n";
    html += "    <meta name='viewport' content='width=device-width, initial-scale=1.0'>\n";
    html += "    <title>Обновление расписания - Ruznama Table</title>\n";
    html += "    <style>\n";
    html += "        * {\n";
    html += "            margin: 0;\n";
    html += "            padding: 0;\n";
    html += "            box-sizing: border-box;\n";
    html += "        }\n";
    html += "        body {\n";
    html += "            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;\n";
    html += "            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);\n";
    html += "            min-height: 100vh;\n";
    html += "            display: flex;\n";
    html += "            flex-direction: column;\n";
    html += "            align-items: center;\n";
    html += "            justify-content: center;\n";
    html += "            padding: 20px;\n";
    html += "        }\n";
    html += "        .container {\n";
    html += "            background: rgba(255, 255, 255, 0.95);\n";
    html += "            border-radius: 20px;\n";
    html += "            box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);\n";
    html += "            padding: 40px;\n";
    html += "            max-width: 500px;\n";
    html += "            width: 100%;\n";
    html += "            text-align: center;\n";
    html += "        }\n";
    html += "        h1 {\n";
    html += "            color: #667eea;\n";
    html += "            margin-bottom: 10px;\n";
    html += "            font-size: 28px;\n";
    html += "        }\n";
    html += "        .subtitle {\n";
    html += "            color: #666;\n";
    html += "            margin-bottom: 30px;\n";
    html += "            font-size: 14px;\n";
    html += "        }\n";
    html += "        .upload-area {\n";
    html += "            border: 2px dashed #667eea;\n";
    html += "            border-radius: 12px;\n";
    html += "            padding: 40px 20px;\n";
    html += "            margin-bottom: 20px;\n";
    html += "            cursor: pointer;\n";
    html += "            transition: all 0.3s;\n";
    html += "        }\n";
    html += "        .upload-area:hover {\n";
    html += "            background: rgba(102, 126, 234, 0.05);\n";
    html += "            border-color: #764ba2;\n";
    html += "        }\n";
    html += "        .upload-icon {\n";
    html += "            font-size: 48px;\n";
    html += "            color: #667eea;\n";
    html += "            margin-bottom: 15px;\n";
    html += "        }\n";
    html += "        .upload-text {\n";
    html += "            color: #333;\n";
    html += "            font-size: 16px;\n";
    html += "            margin-bottom: 5px;\n";
    html += "        }\n";
    html += "        .upload-hint {\n";
    html += "            color: #999;\n";
    html += "            font-size: 12px;\n";
    html += "        }\n";
    html += "        input[type='file'] {\n";
    html += "            display: none;\n";
    html += "        }\n";
    html += "        .btn {\n";
    html += "            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);\n";
    html += "            color: white;\n";
    html += "            border: none;\n";
    html += "            border-radius: 8px;\n";
    html += "            padding: 12px 30px;\n";
    html += "            font-size: 16px;\n";
    html += "            font-weight: 600;\n";
    html += "            cursor: pointer;\n";
    html += "            transition: transform 0.2s, box-shadow 0.2s;\n";
    html += "            width: 100%;\n";
    html += "        }\n";
    html += "        .btn:hover {\n";
    html += "            transform: translateY(-2px);\n";
    html += "            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);\n";
    html += "        }\n";
    html += "        .btn:disabled {\n";
    html += "            background: #ccc;\n";
    html += "            cursor: not-allowed;\n";
    html += "            transform: none;\n";
    html += "            box-shadow: none;\n";
    html += "        }\n";
    html += "        .progress-container {\n";
    html += "            margin-top: 20px;\n";
    html += "            display: none;\n";
    html += "        }\n";
    html += "        .progress-bar {\n";
    html += "            background: #e0e0e0;\n";
    html += "            border-radius: 10px;\n";
    html += "            height: 20px;\n";
    html += "            overflow: hidden;\n";
    html += "        }\n";
    html += "        .progress-fill {\n";
    html += "            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);\n";
    html += "            height: 100%;\n";
    html += "            width: 0%;\n";
    html += "            transition: width 0.3s;\n";
    html += "        }\n";
    html += "        .progress-text {\n";
    html += "            margin-top: 10px;\n";
    html += "            color: #666;\n";
    html += "            font-size: 14px;\n";
    html += "        }\n";
    html += "        .status-message {\n";
    html += "            margin-top: 15px;\n";
    html += "            padding: 10px;\n";
    html += "            border-radius: 8px;\n";
    html += "            font-size: 14px;\n";
    html += "            display: none;\n";
    html += "        }\n";
    html += "        .status-success {\n";
    html += "            background: #d4edda;\n";
    html += "            color: #155724;\n";
    html += "        }\n";
    html += "        .status-error {\n";
    html += "            background: #f8d7da;\n";
    html += "            color: #721c24;\n";
    html += "        }\n";
    html += "        .status-updating {\n";
    html += "            background: #d1ecf1;\n";
    html += "            color: #0c5460;\n";
    html += "        }\n";
    html += "        .info {\n";
    html += "            margin-top: 20px;\n";
    html += "            padding: 15px;\n";
    html += "            background: #e7f3ff;\n";
    html += "            border-left: 4px solid #2196F3;\n";
    html += "            border-radius: 8px;\n";
    html += "            text-align: left;\n";
    html += "        }\n";
    html += "        .info-title {\n";
    html += "            font-weight: bold;\n";
    html += "            color: #0d47a1;\n";
    html += "            margin-bottom: 5px;\n";
    html += "        }\n";
    html += "        .info-text {\n";
    html += "            color: #0d47a1;\n";
    html += "            font-size: 13px;\n";
    html += "            line-height: 1.5;\n";
    html += "        }\n";
    html += "        .back-link {\n";
    html += "            margin-top: 20px;\n";
    html += "            color: #667eea;\n";
    html += "            text-decoration: none;\n";
    html += "            font-size: 14px;\n";
    html += "        }\n";
    html += "        .back-link:hover {\n";
    html += "            text-decoration: underline;\n";
    html += "        }\n";
    html += "        @media (max-width: 480px) {\n";
    html += "            .container {\n";
    html += "                padding: 30px 20px;\n";
    html += "            }\n";
    html += "            h1 {\n";
    html += "                font-size: 24px;\n";
    html += "            }\n";
    html += "        }\n";
    html += "    </style>\n";
    html += "    <script>\n";
    html += "        let selectedFile = null;\n";
    html += "        let updateInProgress = false;\n";
    html += "        \n";
    html += "        // Ждём загрузки DOM перед добавлением обработчиков\n";
    html += "        document.addEventListener('DOMContentLoaded', function() {\n";
    html += "            // Обработка клика на область загрузки\n";
    html += "            document.querySelector('.upload-area').addEventListener('click', function() {\n";
    html += "                if (!updateInProgress) {\n";
    html += "                    document.getElementById('ruznamaFile').click();\n";
    html += "                }\n";
    html += "            });\n";
    html += "            \n";
    html += "            // Обработка выбора файла\n";
    html += "            document.getElementById('ruznamaFile').addEventListener('change', function(e) {\n";
    html += "                if (e.target.files.length > 0) {\n";
    html += "                    selectedFile = e.target.files[0];\n";
    html += "                    document.querySelector('.upload-text').textContent = selectedFile.name;\n";
    html += "                    document.querySelector('.upload-hint').textContent = 'Размер: ' + formatFileSize(selectedFile.size);\n";
    html += "                    document.getElementById('uploadBtn').disabled = false;\n";
    html += "                }\n";
    html += "            });\n";
    html += "            \n";
    html += "            // Загрузка файла\n";
    html += "            document.getElementById('uploadBtn').addEventListener('click', function() {\n";
    html += "                if (selectedFile && !updateInProgress) {\n";
    html += "                    uploadRuznama();\n";
    html += "                }\n";
    html += "            });\n";
    html += "        });\n";
    html += "        \n";
    html += "        // Форматирование размера файла\n";
    html += "        function formatFileSize(bytes) {\n";
    html += "            if (bytes === 0) return '0 Bytes';\n";
    html += "            const k = 1024;\n";
    html += "            const sizes = ['Bytes', 'KB', 'MB', 'GB'];\n";
    html += "            const i = Math.floor(Math.log(bytes) / Math.log(k));\n";
    html += "            return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];\n";
    html += "        }\n";
    html += "        \n";
    html += "        function uploadRuznama() {\n";
    html += "            updateInProgress = true;\n";
    html += "            document.getElementById('uploadBtn').disabled = true;\n";
    html += "            document.getElementById('uploadBtn').textContent = 'Загрузка...';\n";
    html += "            document.querySelector('.progress-container').style.display = 'block';\n";
    html += "            showStatus('updating', 'Обновление расписания...');\n";
    html += "            \n";
    html += "            const formData = new FormData();\n";
    html += "            formData.append('ruznama', selectedFile);\n";
    html += "            \n";
    html += "            fetch('/ruznamaupdate', {\n";
    html += "                method: 'POST',\n";
    html += "                body: formData\n";
    html += "            })\n";
    html += "            .then(response => response.json())\n";
    html += "            .then(data => {\n";
    html += "                if (data.success) {\n";
    html += "                    showStatus('success', data.message);\n";
    html += "                    document.querySelector('.progress-fill').style.width = '100%';\n";
    html += "                    document.querySelector('.progress-text').textContent = '100%';\n";
    html += "                    resetUpload();\n";
    html += "                } else {\n";
    html += "                    showStatus('error', data.message);\n";
    html += "                    resetUpload();\n";
    html += "                }\n";
    html += "            })\n";
    html += "            .catch(err => {\n";
    html += "                console.error('Ошибка:', err);\n";
    html += "                showStatus('error', 'Ошибка соединения');\n";
    html += "                resetUpload();\n";
    html += "            });\n";
    html += "            \n";
    html += "            // Отслеживание прогресса\n";
    html += "            const progressInterval = setInterval(function() {\n";
    html += "                fetch('/ruznamaupdate/status')\n";
    html += "                    .then(response => response.json())\n";
    html += "                    .then(data => {\n";
    html += "                        document.querySelector('.progress-fill').style.width = data.percent + '%';\n";
    html += "                        document.querySelector('.progress-text').textContent = data.percent + '%';\n";
    html += "                        \n";
    html += "                        if (data.status === 'error') {\n";
    html += "                            showStatus('error', data.message || 'Ошибка обновления');\n";
    html += "                            clearInterval(progressInterval);\n";
    html += "                            resetUpload();\n";
    html += "                        } else if (data.status === 'success') {\n";
    html += "                            clearInterval(progressInterval);\n";
    html += "                        }\n";
    html += "                    })\n";
    html += "                    .catch(err => console.log('Ошибка получения статуса:', err));\n";
    html += "            }, 500);\n";
    html += "        }\n";
    html += "        \n";
    html += "        function showStatus(type, message) {\n";
    html += "            const statusEl = document.querySelector('.status-message');\n";
    html += "            statusEl.className = 'status-message status-' + type;\n";
    html += "            statusEl.textContent = message;\n";
    html += "            statusEl.style.display = 'block';\n";
    html += "        }\n";
    html += "        \n";
    html += "        function resetUpload() {\n";
    html += "            updateInProgress = false;\n";
    html += "            document.getElementById('uploadBtn').disabled = true;\n";
    html += "            document.getElementById('uploadBtn').textContent = 'Загрузить расписание';\n";
    html += "            selectedFile = null;\n";
    html += "            document.querySelector('.upload-text').textContent = 'Нажмите для выбора файла';\n";
    html += "            document.querySelector('.upload-hint').textContent = 'Поддерживаются файлы .json';\n";
    html += "            document.getElementById('ruznamaFile').value = '';\n";
    html += "        }\n";
    html += "    </script>\n";
    html += "</head>\n";
    html += "<body>\n";
    html += "    <div class='container'>\n";
    html += "        <h1>Обновление расписания</h1>\n";
    html += "        <div class='subtitle'>Ruznama Table ESP32-S2</div>\n";
    html += "        \n";
    html += "        <div class='upload-area'>\n";
    html += "            <div class='upload-icon'>📅</div>\n";
    html += "            <div class='upload-text'>Нажмите для выбора файла</div>\n";
    html += "            <div class='upload-hint'>Поддерживаются файлы .json</div>\n";
    html += "            <input type='file' id='ruznamaFile' accept='.json'>\n";
    html += "        </div>\n";
    html += "        \n";
    html += "        <button class='btn' id='uploadBtn' disabled>Загрузить расписание</button>\n";
    html += "        \n";
    html += "        <div class='progress-container'>\n";
    html += "            <div class='progress-bar'>\n";
    html += "                <div class='progress-fill'></div>\n";
    html += "            </div>\n";
    html += "            <div class='progress-text'>0%</div>\n";
    html += "        </div>\n";
    html += "        \n";
    html += "        <div class='status-message'></div>\n";
    html += "        \n";
    html += "        <div class='info'>\n";
    html += "            <div class='info-title'>ℹ️ Информация</div>\n";
    html += "            <div class='info-text'>\n";
    html += "                • Выберите файл year_schedule.json<br>\n";
    html += "                • Старый файл будет заменен новым<br>\n";
    html += "                • Расписание будет автоматически перезагружено<br>\n";
    html += "                • После загрузки изменения вступят в силу немедленно\n";
    html += "            </div>\n";
    html += "        </div>\n";
    html += "        \n";
    html += "        <a href='/' class='back-link'>← Вернуться на главную</a>\n";
    html += "    </div>\n";
    html += "</body>\n";
    html += "</html>\n";
    
    return html;
}

// ============================================
// Обработчик GET запроса для страницы обновления прошивки
// ============================================
void handleFirmwareUpdateGET() {
    Serial.println("Web: Запрос страницы обновления прошивки");
    String html = generateFirmwareUpdateHTML();
    server.send(200, "text/html", html);
}

// ============================================
// Обработчик POST запроса для загрузки прошивки
// ============================================
void handleFirmwareUpdatePOST() {
    Serial.println("Web: Запрос на обновление прошивки");
    
    HTTPUpload& upload = server.upload();
    
    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Firmware: Начало загрузки файла: %s\n", upload.filename.c_str());
        Serial.printf("Firmware: Размер файла: %u байт\n", upload.totalSize);
        
        // Проверяем расширение файла
        String filename = upload.filename;
        if (!filename.endsWith(".bin")) {
            Serial.println("Firmware: Неверное расширение файла");
            
            String response = "{\n";
            response += "  \"success\": false,\n";
            response += "  \"message\": \"Файл должен иметь расширение .bin\"\n";
            response += "}";
            
            firmwareStatus = "error";
            firmwareErrorMessage = "Файл должен иметь расширение .bin";
            server.send(400, "application/json", response);
            return;
        }
        
        // Сбрасываем статус
        firmwareStatus = "updating";
        firmwareProgress = 0;
        firmwareErrorMessage = "";
        
        // Проверяем размер файла (ESP32-S2 имеет ограничение на размер прошивки)
        if (upload.totalSize > 2000000) { // Максимально 2MB
            Serial.println("Firmware: Файл слишком большой");
            
            String response = "{\n";
            response += "  \"success\": false,\n";
            response += "  \"message\": \"Файл слишком большой (максимум 2MB)\"\n";
            response += "}";
            
            firmwareStatus = "error";
            firmwareErrorMessage = "Файл слишком большой";
            server.send(400, "application/json", response);
            return;
        }
        
        // Инициализируем обновление
        Serial.println("Firmware: Инициализация обновления...");
        if (!Update.begin(upload.totalSize)) {
            Serial.println("Firmware: Ошибка инициализации обновления");
            Serial.printf("Firmware: Ошибка: %d\n", Update.getError());
            
            String response = "{\n";
            response += "  \"success\": false,\n";
            response += "  \"message\": \"Ошибка инициализации обновления: \" + String(Update.getError())\n";
            response += "}";
            
            firmwareStatus = "error";
            firmwareErrorMessage = "Ошибка инициализации обновления";
            server.send(500, "application/json", response);
            return;
        }
        
        // Устанавливаем callback для отслеживания прогресса
        Update.onProgress([](size_t written, size_t total) {
            if (total > 0) {
                firmwareProgress = (written * 100) / total;
                Serial.printf("Firmware: Прогресс обновления: %u%% (%u/%u байт)\n", firmwareProgress, written, total);
            }
        });
        
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        // Записываем chunk данных в обновление
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Serial.println("Firmware: Ошибка записи chunk данных обновления");
            Serial.printf("Firmware: Попытка записать %u байт, записано %u байт\n", upload.currentSize, Update.write(upload.buf, upload.currentSize));
            
            String response = "{\n";
            response += "  \"success\": false,\n";
            response += "  \"message\": \"Ошибка записи данных обновления\"\n";
            response += "}";
            
            firmwareStatus = "error";
            firmwareErrorMessage = "Ошибка записи данных обновления";
            server.send(500, "application/json", response);
            Update.abort();
            return;
        }
        
        // Обновляем прогресс (уже делается в callback)
        Serial.printf("Firmware: Записан chunk из %u байт\n", upload.currentSize);
        
    } else if (upload.status == UPLOAD_FILE_END) {
        Serial.printf("Firmware: Загрузка завершена. Размер: %u байт\n", upload.totalSize);
        
        // Завершаем обновление
        if (Update.end(true)) {
            Serial.println("Firmware: Обновление завершено успешно");
            
            String response = "{\n";
            response += "  \"success\": true,\n";
            response += "  \"message\": \"Обновление завершено успешно\"\n";
            response += "}";
            
            firmwareStatus = "success";
            firmwareProgress = 100;
            
            // Отправляем ответ клиенту
            server.send(200, "application/json", response);
            
            Serial.println("Firmware: Перезагрузка устройства...");
            delay(1000); // Даем время на отправку ответа
            ESP.restart();
        } else {
            Serial.printf("Firmware: Ошибка завершения обновления: %d\n", Update.getError());
            
            String response = "{\n";
            response += "  \"success\": false,\n";
            response += "  \"message\": \"Ошибка завершения обновления: \" + String(Update.getError())\n";
            response += "}";
            
            firmwareStatus = "error";
            firmwareErrorMessage = "Ошибка завершения обновления";
            server.send(500, "application/json", response);
        }
    }
}

// ============================================
// Обработчик API для получения статуса обновления прошивки
// ============================================
void handleFirmwareUpdateStatus() {
    String json = "{\n";
    json += "  \"status\": \"" + firmwareStatus + "\",\n";
    json += "  \"percent\": " + String(firmwareProgress);
    
    if (firmwareStatus == "error" && firmwareErrorMessage.length() > 0) {
        json += ",\n";
        json += "  \"message\": \"" + firmwareErrorMessage + "\"";
    }
    
    json += "\n}";
    
    server.send(200, "application/json", json);
}

// ============================================
// Инициализация точки доступа WiFi
// ============================================
void wifiApInit() {
    Serial.println("\n========================================");
    Serial.println("Инициализация точки доступа WiFi...");
    Serial.println("========================================\n");
    
    // Настройка точки доступа
    WiFi.mode(WIFI_AP);
    
    Serial.print("Создание точки доступа '");
    Serial.print(WIFI_AP_SSID);
    Serial.println("'...");
    
    if (strlen(WIFI_AP_PASSWORD) > 0) {
        WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CONNECTIONS);
    } else {
        WiFi.softAP(WIFI_AP_SSID);
    }
    
    // Вывод IP адреса
    IPAddress IP = WiFi.softAPIP();
    Serial.print("IP адрес точки доступа: ");
    Serial.println(IP);
    
    // Настройка DNS сервера для Captive Portal
    Serial.println("\nНастройка DNS сервера для Captive Portal...");
    dnsServer.start(DNS_PORT, "*", IP);
    Serial.println("DNS сервер запущен на порту " + String(DNS_PORT));
    
    // Настройка веб-сервера
    Serial.println("\nНастройка веб-сервера...");
    
    server.on("/", handleRoot);
    server.on("/time", handleTimeAPI);
    server.on("/settime", HTTP_POST, handleSetTimeAPI);
    server.on("/hijri-offset", HTTP_POST, handleHijriOffsetAPI);
    server.on("/ruznamaupdate", HTTP_GET, handleRuznamaUpdateGET);
    server.on("/ruznamaupdate", HTTP_POST, handleRuznamaUpdatePOST);
    server.on("/ruznamaupdate/status", handleRuznamaUpdateStatus);
    server.on("/firmwareupdate", HTTP_GET, handleFirmwareUpdateGET);
    server.on("/firmwareupdate", HTTP_POST, handleFirmwareUpdatePOST);
    server.on("/firmwareupdate/status", handleFirmwareUpdateStatus);
    server.onNotFound(handleNotFound);
    
    server.begin();
    Serial.println("Веб-сервер запущен на порту " + String(WEB_SERVER_PORT));
    
    Serial.println("\n========================================");
    Serial.println("Точка доступа WiFi готова!");
    Serial.println("SSID: " + String(WIFI_AP_SSID));
    Serial.println("IP: " + IP.toString());
    Serial.println("URL: http://" + IP.toString());
    Serial.println("Captive Portal: ВКЛЮЧЕН");
    Serial.println("========================================\n");
}

// ============================================
// Обработка запросов веб-сервера
// ============================================
void wifiApHandleClient() {
    // Обрабатываем DNS запросы для Captive Portal
    dnsServer.processNextRequest();
    
    // Обрабатываем HTTP запросы веб-сервера
    server.handleClient();
}

// ============================================
// Получить IP адрес точки доступа
// ============================================
String wifiApGetIP() {
    return WiFi.softAPIP().toString();
}

// ============================================
// Проверить статус точки доступа
// ============================================
bool wifiApIsConnected() {
    return WiFi.softAPgetStationNum() > 0;
}
