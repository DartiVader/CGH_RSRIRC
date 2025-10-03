// работа кода Serial → LittleFS → Wi-Fi → Web Server

#include <Arduino.h>
#include "config.h"
#include "WIFI_App/WIFI_App.h"
#include "web_server/web_server.h"
#include <LittleFS.h>

PositionData testPosition;

// Функция для надежной инициализации LittleFS,
bool initializeLittleFS() {
    Serial.println("Initializing LittleFS...");
    
    // Ищет существующую файловую систему во флеш-памяти ESP32
    if (LittleFS.begin()) {
        Serial.println("+ LittleFS mounted successfully");
        
        // Проверяем, есть ли необходимые файлы
        if (LittleFS.exists("/index.html")) {
            Serial.println("+ Web files found");
            return true;
        } else {
            Serial.println("- LittleFS mounted but web files not found");
        }
    }
    
    // Если монтирование не удалось - форматируем
    Serial.println("- LittleFS mount failed, formatting...");
    
    if (LittleFS.format()) {
        Serial.println("+ LittleFS formatted successfully");
        
        // Пытаемся смонтировать снова после форматирования
        if (LittleFS.begin()) {
            Serial.println("+ LittleFS mounted after format");
            return true;
        }
    }
    
    Serial.println("- Critical: LittleFS initialization failed!");
    return false;
}

//Тестовые данные, заменить для данных получаемых с датчиков!!!
void generateTestData() {
    static unsigned long lastUpdate = 0;
    static float angle = 0;
    
    if (millis() - lastUpdate > 200) { // 5 Гц
        // Генерация тестовой траектории (круг)
        testPosition.x = 200 + 100 * cos(angle);
        testPosition.y = 150 + 100 * sin(angle);
        testPosition.accuracy = 2.5 + (rand() % 150) / 100.0;
        testPosition.timestamp = millis();
        testPosition.valid = true;
        
        // Обновление в веб-сервисе
        webService.updatePosition(testPosition);
        
        angle += 0.1;
        if (angle > 2 * PI) angle = 0;
        lastUpdate = millis();
    }
}

void setup() {
    // Инициализация порта для терминала с скоростью передачи 115200 
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n Starting Positioning System - Stage 1");
    Serial.println("=======================================");
    
    // 1. Инициализация файловой системы (ДО всего остального)
    if (!initializeLittleFS()) {
        Serial.println("- Cannot continue without filesystem!");
        while(1) {
            delay(1000);
            Serial.println("- Filesystem error");
        }
    }
    
    // 2. Инициализация Wi-Fi
    Serial.println("\n + Starting WiFi Access Point...");
    wifiApp.begin();
    
    // 3. Инициализация веб-сервера
    Serial.println("+ Starting Web Server...");
    webService.begin();
    
    Serial.println("\n + System ready!");
    Serial.println(" + Connect to WiFi: " + String(WIFI_SSID));
    Serial.println(" + Open in browser: http://" + WiFi.softAPIP().toString());
}

void loop() {
    generateTestData();
    delay(10); 
}