#ifndef CONFIG_H
#define CONFIG_H

// Сеть
#define WIFI_SSID "PositioningSystem"
#define WIFI_PASSWORD "password123"
#define UDP_PORT 1234
#define WEB_PORT 80

// Пины
#define STATUS_LED_PIN 2
#define ULTRASOUND_TX_PIN 25
#define ULTRASOUND_RX_PIN 26

// Позиции маяков (в см)
const float BEACON_POSITIONS[3][2] = {
    {0.0, 0.0},     // Маяк 1: левый нижний угол
    {400.0, 0.0},   // Маяк 2: правый нижний угол
    {200.0, 300.0}  // Маяк 3: верхний центр
};

// Физика
#define SOUND_SPEED 34300.0    // см/с при 20°C
#define ULTRASOUND_FREQ 40000  // 40 kHz

#endif
