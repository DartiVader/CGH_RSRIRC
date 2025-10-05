#ifndef CONFIG_H
#define CONFIG_H

// ====================
// БАЗОВЫЕ НАСТРОЙКИ ПИНОВ
// ====================
#ifndef STATUS_LED_PIN
#define STATUS_LED_PIN 2
#endif

#ifndef ULTRASOUND_TX_PIN
#define ULTRASOUND_TX_PIN 25
#endif

#ifndef ULTRASOUND_RX_PIN
#define ULTRASOUND_RX_PIN 35
#endif

// ====================
// НАСТРОЙКИ СЕТИ
// ====================
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
    float x = 0;
    float y = 0;
    float accuracy = 0;
    uint32_t timestamp = 0;
    bool valid = false;
};

// ====================
// ФИЗИЧЕСКИЕ ПАРАМЕТРЫ
// ====================
#define SOUND_SPEED 34300.0

// ====================
// ПОЗИЦИИ УЗЛОВ (см)
// ====================
#define RECEIVER_X 0
#define RECEIVER_Y 0

#define BEACON1_X -200
#define BEACON1_Y 300

#define BEACON2_X 200
#define BEACON2_Y 300

#define BEACON3_X 0
#define BEACON3_Y 0

// ====================
// ТАЙМИНГИ (мс)
// ====================
#define BEACON_DELAY 200
#define MEASUREMENT_DELAY 100
#define PULSE_DURATION 10
#define TIMEOUT 10000

// ====================
// КОНФИГУРАЦИИ УЗЛОВ
// ====================

#ifdef RECEIVER_NODE
// КОНФИГУРАЦИЯ ПРИЕМНИКА
#define SENSOR_TYPE "KY-037"
#define NODE_TYPE "RECEIVER"
#define SENSOR_PIN 35

#elif defined(BEACON_NODE)
// КОНФИГУРАЦИЯ МАЯКОВ
#if BEACON_ID == 1
#define SENSOR_TYPE "KY-006"  // ИСПРАВЛЕНО
#define NODE_TYPE "BEACON_1"
#define SENSOR_PIN 34
#elif BEACON_ID == 2
#define SENSOR_TYPE "KY-006"  // ИСПРАВЛЕНО
#define NODE_TYPE "BEACON_2"
#define SENSOR_PIN 36
#endif

#elif defined(OBJECT_NODE)
// КОНФИГУРАЦИЯ ОБЪЕКТА
#define SENSOR_TYPE "DYNAMIC+PAM8403"
#define NODE_TYPE "OBJECT"

// Параметры ультразвуковых импульсов объекта
#define PULSE_DURATION 15
#define BETWEEN_PULSE_DELAY 10
#define OBJECT_ID 3
#define OBJECT_PULSE_INTERVAL 2000

// Переопределяем пины для Arduino Uno
#undef STATUS_LED_PIN
#undef ULTRASOUND_TX_PIN

#define STATUS_LED_PIN 13
#define ULTRASOUND_TX_PIN 8

#endif

#endif // CONFIG_H
