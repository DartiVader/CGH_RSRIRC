#include <Arduino.h>
#include "config/config.h"
#include "positioning/positioning.h"
#include "network/wifi_manager.h"
#include "hardware/ultrasound.h"

// ===== РЕАЛИЗАЦИЯ РЕСИВЕРА =====
#ifdef RECEIVER_NODE

// Объявляем экземпляры классов для ресивера
PositioningSystem positioning;
WiFiManager wifi;
Ultrasound ultrasound;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n=== 🚀 POSITIONING SYSTEM - RECEIVER ===");

    // Инициализация компонентов
    Serial.println("Initializing positioning system...");
    positioning.begin();

    Serial.println("Starting WiFi Access Point...");
    wifi.setupAP();

    Serial.println("Starting UDP server...");
    wifi.startUDP();

    Serial.println("Setting up ultrasound receiver...");
    ultrasound.setupReceiver();

    // Настройка пинов
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, HIGH);

    Serial.println("✅ Receiver initialization completed!");
    Serial.println("📡 Waiting for beacons to connect...");
}

void loop() {
    // Мигаем светодиодом (индикация работы)
    static unsigned long lastLedToggle = 0;
    if (millis() - lastLedToggle > 500) {
        digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
        lastLedToggle = millis();
    }

    static unsigned long lastBeaconCheck = 0;
    static unsigned long lastPositionUpdate = 0;

    // Проверяем подключенные маяки каждые 3 секунды
    if (millis() - lastBeaconCheck > 3000) {
        lastBeaconCheck = millis();
        int stations = WiFi.softAPgetStationNum();
        Serial.println("📡 Connected beacons: " + String(stations));

        if (stations > 0) {
            wifi.sendUDPBroadcast("START");
            Serial.println("📤 Sent START command to all beacons");
        } else {
            Serial.println("⏳ Waiting for beacons to connect...");
        }
    }

    // Проверяем ультразвуковые импульсы
    if (ultrasound.detectPulse()) {
        unsigned long pulseTime = micros();
        Serial.println("🎯 Ultrasound pulse detected! Time: " + String(pulseTime));
        // Здесь будем фиксировать время прихода импульса для TDOA
    }

    // Обновление позиции каждые 5 секунд
    if (millis() - lastPositionUpdate > 5000) {
        lastPositionUpdate = millis();
        positioning.update();
        Position pos = positioning.getCurrentPosition();

        Serial.printf("📍 Position: X=%.1fcm, Y=%.1fcm, Accuracy=%.1fcm\n",
                     pos.x, pos.y, pos.accuracy);
    }

    delay(50);
}

// ===== РЕАЛИЗАЦИЯ МАЯКА =====
#elif BEACON_NODE

// Объявляем экземпляры классов для маяка
WiFiManager wifi;
Ultrasound ultrasound;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n=== 🎯 POSITIONING SYSTEM - BEACON ===");
    Serial.println("Beacon ID: " + String(BEACON_ID));

    // Инициализация компонентов
    Serial.println("Connecting to WiFi...");
    wifi.setupSTA();

    Serial.println("Starting UDP client...");
    wifi.startUDP();

    Serial.println("Setting up ultrasound transmitter...");
    ultrasound.setupTransmitter();

    // Настройка пинов
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, HIGH);

    Serial.println("✅ Beacon initialization completed!");
    Serial.println("📡 Connected to: " + String(WiFi.SSID()));
    Serial.println("📶 IP address: " + WiFi.localIP().toString());
}

void loop() {
    // Мигаем светодиодом (индикация работы)
    static unsigned long lastLedToggle = 0;
    if (millis() - lastLedToggle > 1000) {
        digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
        lastLedToggle = millis();
    }

    // Проверяем входящие UDP сообщения
    int packetSize = wifi.udp.parsePacket();
    if (packetSize) {
        char packet[50];
        int len = wifi.udp.read(packet, 50);
        if (len > 0) {
            packet[len] = 0;
            String message = String(packet);

            Serial.println("📨 Received: " + message);

            if (message == "START") {
                Serial.println("🚀 Starting ultrasound pulse...");

                // Небольшая задержка для предотвращения интерференции
                delay(random(50, 200));

                // Излучаем ультразвуковой импульс
                ultrasound.emitPulse(BEACON_ID);
                Serial.println("✅ Pulse emitted with ID: " + String(BEACON_ID));

                // Отправляем подтверждение
                String ackMessage = "ACK_BEACON_" + String(BEACON_ID);
                wifi.sendUDPBroadcast(ackMessage);
                Serial.println("📤 Sent: " + ackMessage);
            }
        }
    }

    // Проверяем статус WiFi каждые 15 секунд
    static unsigned long lastWifiCheck = 0;
    if (millis() - lastWifiCheck > 15000) {
        lastWifiCheck = millis();
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("🔄 WiFi disconnected! Attempting reconnect...");
            wifi.setupSTA();
        } else {
            Serial.println("📶 WiFi: Connected to " + String(WiFi.SSID()));
        }
    }

    delay(100);
}

// ===== ЕСЛИ НИ ОДНА КОНФИГУРАЦИЯ НЕ ВЫБРАНА =====
#else
#error "Please define either RECEIVER_NODE or BEACON_NODE in build flags"
#endif
