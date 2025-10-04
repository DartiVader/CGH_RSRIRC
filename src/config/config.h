#ifndef CONFIG_H
#define CONFIG_H

// Пины по умолчанию
#ifndef STATUS_LED_PIN
#define STATUS_LED_PIN 2
#endif

#ifndef ULTRASOUND_TX_PIN
#define ULTRASOUND_TX_PIN 25
#endif

#ifndef ULTRASOUND_RX_PIN
#define ULTRASOUND_RX_PIN 35
#endif

// Сеть
#define WIFI_SSID "PositioningSystem"
#define WIFI_PASSWORD "password123"
#define WIFI_CHANNEL 1
#define MAX_CLIENTS 4

// ====================
// НАСТРОЙКИ СЕРВЕРОВ
// ====================
#define WEB_PORT 80
#define WS_PORT 81
#define WS_PATH "/ws"

// ====================
// НАСТРОЙКИ СИСТЕМЫ
// ====================
#define MEASUREMENT_RATE 5
#define BUFFER_SIZE 256
#define UDP_PORT 1234

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


// Параметры для разных узлов
#ifdef RECEIVER_NODE
#define SENSOR_TYPE "KY-037"
#define NODE_TYPE "RECEIVER"

#elif BEACON_NODE
  #if BEACON_ID == 1
  #define SENSOR_TYPE "KY-006+KY-037"
  #define NODE_TYPE "BEACON_1"
  #elif BEACON_ID == 2
  #define SENSOR_TYPE "KY-012+KY-038"
  #define NODE_TYPE "BEACON_2"
  #endif

#elif OBJECT_NODE
#define SENSOR_TYPE "KY-006"
#define NODE_TYPE "OBJECT"
#define OBJECT_PULSE_INTERVAL 1000000  // 1 секунда между импульсами

#endif

// Физические параметры
#define SOUND_SPEED 34300.0  // см/с

#endif
