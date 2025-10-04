#ifndef CONFIG_H
#define CONFIG_H

// ====================
// БАЗОВЫЕ НАСТРОЙКИ ПИНОВ
// ====================

// Пины по умолчанию (для ESP32 узлов)
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
    float x = 0;              // X координата (см)
    float y = 0;              // Y координата (см)
    float accuracy = 0;       // Точность (см)
    uint32_t timestamp = 0;   // Метка времени
    bool valid = false;       // Валидность данных
};

// ====================
// ФИЗИЧЕСКИЕ ПАРАМЕТРЫ
// ====================
#define SOUND_SPEED 34300.0  // см/с

// ====================
// КОНФИГУРАЦИИ УЗЛОВ
// ====================

#ifdef RECEIVER_NODE
// КОНФИГУРАЦИЯ ПРИЕМНИКА (ESP32)
#define SENSOR_TYPE "KY-037"
#define NODE_TYPE "RECEIVER"

#elif BEACON_NODE
// КОНФИГУРАЦИЯ МАЯКОВ (ESP32)
  #if BEACON_ID == 1
  #define SENSOR_TYPE "KY-006+KY-037"
  #define NODE_TYPE "BEACON_1"
  #elif BEACON_ID == 2
  #define SENSOR_TYPE "KY-012+KY-038"
  #define NODE_TYPE "BEACON_2"
  #endif

#elif OBJECT_NODE
// КОНФИГУРАЦИЯ ОБЪЕКТА (Arduino Uno с усилителем)
#define SENSOR_TYPE "DYNAMIC+PAM8403"
#define NODE_TYPE "OBJECT"

// Переопределяем пины для Arduino Uno
#undef STATUS_LED_PIN
#undef ULTRASOUND_TX_PIN

#define STATUS_LED_PIN 13
#define ULTRASOUND_TX_PIN 8     // Подключен к L усилителя

// Параметры ультразвуковых импульсов
#define PULSE_DURATION 15       // длительность импульса в мс
#define BETWEEN_PULSE_DELAY 10  // задержка между импульсами в мс

// Идентификация объекта
#define OBJECT_ID 3             // 3 импульса для идентификации
#define OBJECT_PULSE_INTERVAL 2000  // 2 секунды между импульсами (в миллисекундах)

#endif

#endif
