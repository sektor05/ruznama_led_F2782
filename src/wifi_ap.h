#ifndef WIFI_AP_H
#define WIFI_AP_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <HTTPUpdate.h>
#include <LittleFS.h>

// Настройки точки доступа
#define WIFI_AP_SSID "ruznama_table"
#define WIFI_AP_PASSWORD "ruznama123"  // Точка доступа с паролем
#define WIFI_AP_CHANNEL 1
#define WIFI_AP_MAX_CONNECTIONS 4

// Порт веб-сервера
#define WEB_SERVER_PORT 80

// ============================================
// Инициализация точки доступа WiFi
// ============================================
void wifiApInit();

// ============================================
// Обработка запросов веб-сервера
// ============================================
void wifiApHandleClient();

// ============================================
// Получить IP адрес точки доступа
// ============================================
String wifiApGetIP();

// ============================================
// Проверить статус точки доступа
// ============================================
bool wifiApIsConnected();

#endif // WIFI_AP_H
