#ifndef CONFIG_H
#define CONFIG_H

// === БЕЗОПАСНЫЕ ПИНЫ ДЛЯ ESP32 ===
// ADC1 пины (безопасны с WiFi): 32, 33, 34, 35, 36, 37, 38, 39
// ADC2 пины (конфликтуют с WiFi): 0, 2, 4, 12, 13, 14, 15, 25, 26, 27

// Пины по умолчанию (будут переопределены в platformio.ini)
#ifndef STATUS_LED_PIN
#define STATUS_LED_PIN 2
#endif

#ifndef ULTRASOUND_TX_PIN
#define ULTRASOUND_TX_PIN 25    // ADC2, но для маяков норм (они не читают аналог)
#endif

#ifndef ULTRASOUND_RX_PIN
#define ULTRASOUND_RX_PIN 36    // ADC1 - безопасно для ресивера с WiFi
#endif

// Параметры сети
#ifndef WIFI_SSID
#define WIFI_SSID "PositioningSystem"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "password123"
#endif

#define UDP_PORT 1234

// Позиции маяков (в см)
const float BEACON_POSITIONS[3][2] = {
    {0.0, 0.0},     // Маяк 1
    {400.0, 0.0},   // Маяк 2
    {200.0, 300.0}  // Маяк 3
};

// Физические параметры
#define SOUND_SPEED 34300.0  // см/с

#endif
