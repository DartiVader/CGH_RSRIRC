#ifndef CONFIG_H
#define CONFIG_H
#include <stdint.h>
#include "Arduino.h"
// ==================== 
// НАСТРОЙКИ WI-FI
// ====================
#define WIFI_SSID "Artak" //login
#define WIFI_PASSWORD "position123" //password
#define WIFI_CHANNEL 1 // channel
#define MAX_CLIENTS 4 // max people on wifi

// ====================
// НАСТРОЙКИ СЕРВЕРОВ
// ====================
// ВебСокет для мгновенного обновление данных на сервере и передаче на клиент, легче трафику
// клиент - js , сервер -esp

#define WEB_PORT 80 // port
#define WS_PORT 81 // port for socket
#define WS_PATH "/ws"

// ====================
// НАСТРОЙКИ СИСТЕМЫ
// ====================
#define MEASUREMENT_RATE 5      // 5 Гц translate to another esp
#define BUFFER_SIZE 256 

// ====================
// СТРУКТУРЫ ДАННЫХ
// ====================
struct PositionData {
    float x = 0;              // X координата (см)
    float y = 0;              // Y координата (см) 
    float accuracy = 0;       // Точность (см)
    uint32_t timestamp = 0;   // Метка времени
    bool valid = false;       // Валидность данных
};

#endif