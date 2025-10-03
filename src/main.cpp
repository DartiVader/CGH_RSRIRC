#include <Arduino.h>
#include "config/config.h"
#include "positioning/positioning.h"
#include "network/wifi_manager.h"
#include "hardware/ultrasound.h"

// Объявляем экземпляры классов
PositioningSystem positioning;
WiFiManager wifi;
Ultrasound ultrasound;

// Прототипы функций
void setupReceiver();
void setupBeacon();
void loopReceiver();
void loopBeacon();

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n=== Positioning System ===");

    positioning.begin();

    #ifdef RECEIVER_NODE
    Serial.println("🚀 Starting RECEIVER node");
    wifi.setupAP();
    wifi.startUDP();
    setupReceiver();

    #elif BEACON_NODE
    Serial.println("🎯 Starting BEACON node");
    wifi.setupSTA();
    wifi.startUDP();

    // Инициализируем ультразвук для маяка
    ultrasound.setupTransmitter();

    setupBeacon();
    #endif
}

void loop() {
    #ifdef RECEIVER_NODE
    loopReceiver();

    #elif BEACON_NODE
    loopBeacon();
    #endif
}

void setupReceiver() {
    Serial.println("Receiver setup completed");
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, HIGH);
}

void setupBeacon() {
    Serial.println("Beacon setup completed");
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, HIGH);
}

void loopReceiver() {
    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));

    static unsigned long lastUpdate = 0;
    static unsigned long lastBeaconCheck = 0;

    // Проверяем подключенные устройства каждые 3 секунды
    if (millis() - lastBeaconCheck > 3000) {
        lastBeaconCheck = millis();
        int stations = WiFi.softAPgetStationNum();
        Serial.println("📡 Connected stations: " + String(stations));

        if (stations > 0) {
            wifi.sendUDPBroadcast("START");
            Serial.println("📤 Sent START command to all beacons");
        } else {
            Serial.println("⏳ Waiting for beacons to connect...");
        }
    }

    // Обновление позиции каждые 5 секунд
    if (millis() - lastUpdate > 5000) {
        positioning.update();
        Position pos = positioning.getCurrentPosition();

        Serial.printf("Position: X=%.1fcm, Y=%.1fcm, Accuracy=%.1fcm\n",
                     pos.x, pos.y, pos.accuracy);
        lastUpdate = millis();
    }

    delay(100);
}

void loopBeacon() {
    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));

    // Проверяем UDP пакеты
    int packetSize = wifi.udp.parsePacket();
    if (packetSize) {
        char packet[50];
        int len = wifi.udp.read(packet, 50);
        if (len > 0) {
            packet[len] = 0;

            String message = String(packet);
            Serial.println("📨 Received UDP: " + message);

            if (message == "START") {
                Serial.println("🚀 Starting ultrasound pulse");

                #ifdef BEACON_ID
                ultrasound.emitPulse(BEACON_ID);
                Serial.println("🎯 Emitted pulse with ID: " + String(BEACON_ID));
                #else
                ultrasound.emitPulse(1); // ID по умолчанию
                Serial.println("🎯 Emitted pulse with default ID: 1");
                #endif

                // Отправляем подтверждение
                wifi.sendUDPBroadcast("ACK from beacon");
                Serial.println("✅ Sent ACK confirmation");
            }
        }
    }

    // Проверяем статус WiFi каждые 10 секунд
    static unsigned long lastWifiCheck = 0;
    if (millis() - lastWifiCheck > 10000) {
        lastWifiCheck = millis();
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("🔄 WiFi disconnected! Attempting reconnect...");
            wifi.setupSTA();
        } else {
            Serial.println("📶 WiFi connected to: " + String(WiFi.SSID()) + " | IP: " + WiFi.localIP().toString());
        }
    }

    delay(100);
}
