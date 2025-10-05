#include <Arduino.h>

// ===== РЕАЛИЗАЦИЯ ПРИЕМНИКА =====
#ifdef RECEIVER_NODE
#include <WiFi.h>
#include <WiFiUdp.h>
#include "config/config.h"
#include "positioning/positioning.h"
#include "web_server/web_server.h"
#include "hardware/sensor_reader.h"

WiFiUDP udp;
PositioningSystem positioning;
WebService webService;
SensorReader receiverSensor;

// Состояние системы
bool measurementActive = false;
unsigned long measurementStart = 0;
int currentStep = 0;

// Измеренные расстояния (2 маяка + объект)
float measuredDistances[3] = {0}; // [beacon1, beacon2, object]

// Функции для отправки команд
void sendToBeacon(int beaconId, const String& message) {
    IPAddress targetIP(192, 168, 4, 10 + beaconId); // beacon1: .11, beacon2: .12
    udp.beginPacket(targetIP, UDP_PORT);
    udp.print(message);
    udp.endPacket();
    Serial.println("Sent to Beacon " + String(beaconId) + ": " + message);
}

void sendToObject(const String& message) {
    IPAddress targetIP(192, 168, 4, 20); // object: .20
    udp.beginPacket(targetIP, UDP_PORT);
    udp.print(message);
    udp.endPacket();
    Serial.println("Sent to Object: " + message);
}

void startMeasurement() {
    if (measurementActive) {
        Serial.println("Measurement already in progress");
        return;
    }

    measurementActive = true;
    measurementStart = millis();
    currentStep = 0;
    memset(measuredDistances, 0, sizeof(measuredDistances));

    webService.updateMeasurementStatus(true);
    webService.setActiveBeacon(1);

    Serial.println("=== STARTING 2-BEACON MEASUREMENT CYCLE ===");
    sendToBeacon(1, "START");
}

void stopMeasurement() {
    measurementActive = false;
    webService.updateMeasurementStatus(false);
    webService.setActiveBeacon(-1);
    Serial.println("Measurement stopped");
}

// Функция для детектирования импульсов приемником
void handleReceiverDetection() {
    if (receiverSensor.detectPulse(50000)) {
        unsigned long detectionTime = receiverSensor.getLastPulseTime();
        Serial.println("Receiver detected pulse - Time: " + String(detectionTime) + " μs, Amplitude: " + String(receiverSensor.getLastAmplitude()));
    }
}

void processUDP() {
    int packetSize = udp.parsePacket();
    if (packetSize) {
        char packet[50];
        int len = udp.read(packet, sizeof(packet)-1);
        if (len > 0) {
            packet[len] = 0;
            String message = String(packet);
            String senderIP = udp.remoteIP().toString();

            Serial.println("UDP from " + senderIP + ": " + message);

            if (message.startsWith("BEACON_TIME:")) {
                int beaconId = message.substring(12, 13).toInt();
                unsigned long time = message.substring(14).toInt();

                if (beaconId >= 1 && beaconId <= 2) {
                    measuredDistances[beaconId-1] = (time / 1000000.0) * SOUND_SPEED;
                    Serial.println("Beacon " + String(beaconId) + " distance: " + String(measuredDistances[beaconId-1]) + " cm");

                    if (beaconId == 1 && currentStep == 0) {
                        currentStep = 1;
                        webService.setActiveBeacon(2);
                        sendToBeacon(2, "START");
                    }
                    else if (beaconId == 2 && currentStep == 1) {
                        currentStep = 2;
                        webService.setActiveBeacon(0);
                        sendToObject("START");
                    }
                }
            }
            else if (message.startsWith("OBJECT_TIME:")) {
                unsigned long time = message.substring(12).toInt();
                measuredDistances[2] = (time / 1000000.0) * SOUND_SPEED;
                Serial.println("Object distance: " + String(measuredDistances[2]) + " cm");

                Position objectPos = positioning.trilaterate(measuredDistances);

                Serial.println("=== 2-BEACON POSITION CALCULATED ===");
                Serial.println("X: " + String(objectPos.x) + " cm");
                Serial.println("Y: " + String(objectPos.y) + " cm");
                Serial.println("Accuracy: " + String(objectPos.accuracy) + " cm");

                PositionData webPos;
                webPos.x = objectPos.x;
                webPos.y = objectPos.y;
                webPos.accuracy = objectPos.accuracy;
                webPos.timestamp = millis();
                webPos.valid = (objectPos.accuracy < 50.0);

                webService.updatePosition(webPos);
                stopMeasurement();

                Serial.println("=== 2-BEACON MEASUREMENT COMPLETED ===");
            }
        }
    }
}

void setupWiFi() {
    Serial.println("Starting WiFi Access Point...");

    WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
    WiFi.softAPConfig(
        IPAddress(192, 168, 4, 10),
        IPAddress(192, 168, 4, 10),
        IPAddress(255, 255, 255, 0)
    );

    delay(1000);

    Serial.println("WiFi AP Started!");
    Serial.println("SSID: " + String(WIFI_SSID));
    Serial.println("IP: " + WiFi.softAPIP().toString());
    Serial.println("MAC: " + WiFi.softAPmacAddress());
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== 2-BEACON POSITIONING SYSTEM - RECEIVER NODE ===");

    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW);

    setupWiFi();

    udp.begin(UDP_PORT);
    positioning.begin();
    receiverSensor.begin(SENSOR_PIN, SENSOR_TYPE);
    webService.begin();

    Serial.println("2-Beacon Receiver initialization completed!");
    Serial.println("Available commands:");
    Serial.println("   start  - Begin measurement cycle");
    Serial.println("   stop   - Stop current measurement");
    Serial.println("   status - System status");
    Serial.println("   calibrate - Calibrate sensors");
    Serial.println("   help   - Show this help");

    digitalWrite(STATUS_LED_PIN, HIGH);
}

void loop() {
    processUDP();
    handleReceiverDetection();

    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        if (cmd == "start") {
            startMeasurement();
        }
        else if (cmd == "stop") {
            stopMeasurement();
        }
        else if (cmd == "status") {
            Position currentPos = positioning.getCurrentPosition();
            Serial.println("=== 2-BEACON SYSTEM STATUS ===");
            Serial.println("Position - X: " + String(currentPos.x) + " cm, Y: " + String(currentPos.y) + " cm");
            Serial.println("Accuracy: " + String(currentPos.accuracy) + " cm");
            Serial.println("Connected stations: " + String(WiFi.softAPgetStationNum()));
            Serial.println("Measurement active: " + String(measurementActive ? "YES" : "NO"));
            Serial.println("Current step: " + String(currentStep));
            Serial.println("Uptime: " + String(millis()/1000) + "s");

            Serial.println("Measured distances:");
            Serial.println("  Beacon 1: " + String(measuredDistances[0]) + " cm");
            Serial.println("  Beacon 2: " + String(measuredDistances[1]) + " cm");
            Serial.println("  Object: " + String(measuredDistances[2]) + " cm");
        }
        else if (cmd == "calibrate") {
            Serial.println("Calibrating receiver sensor...");
            receiverSensor.calibrate();
        }
        else if (cmd == "help") {
            Serial.println("=== 2-BEACON AVAILABLE COMMANDS ===");
            Serial.println("start  - Begin measurement cycle");
            Serial.println("stop   - Stop current measurement");
            Serial.println("status - System status");
            Serial.println("calibrate - Calibrate sensors");
            Serial.println("help   - Show this help");
        }
        else if (cmd.length() > 0) {
            Serial.println("Unknown command: " + cmd);
        }
    }

    if (measurementActive && (millis() - measurementStart > TIMEOUT)) {
        Serial.println("Measurement timeout");
        stopMeasurement();
    }

    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 1000) {
        digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
        lastBlink = millis();
    }

    delay(50);
}

// ===== РЕАЛИЗАЦИЯ МАЯКОВ С KY-006 =====
#elif defined(BEACON_NODE)

#include <WiFi.h>
#include <WiFiUdp.h>
#include "config/config.h"

WiFiUDP udp;
int beaconId = BEACON_ID;

// Переменные для измерения времени
unsigned long pulseStartTime = 0;
bool measurementInProgress = false;

void setup() {
    Serial.begin(115200);

    // Настройка пинов для KY-006 (зуммер) и светодиода
    pinMode(ULTRASOUND_TX_PIN, OUTPUT);
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(ULTRASOUND_TX_PIN, LOW);
    digitalWrite(STATUS_LED_PIN, LOW);

    Serial.println("=== BEACON " + String(beaconId) + " WITH KY-006 ===");
    Serial.println("Initializing...");

    // Подключение к WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        udp.begin(UDP_PORT);
        Serial.println("\n✅ Beacon " + String(beaconId) + " ready!");
        Serial.println("📡 IP: " + WiFi.localIP().toString());
        Serial.println("🎵 Type: KY-006 Ultrasound Transmitter");
        Serial.println("📊 Sensor: " + String(SENSOR_TYPE));
        digitalWrite(STATUS_LED_PIN, HIGH);
    } else {
        Serial.println("\n❌ WiFi connection failed!");
        while(1) {
            digitalWrite(STATUS_LED_PIN, HIGH);
            delay(100);
            digitalWrite(STATUS_LED_PIN, LOW);
            delay(100);
        }
    }
}

// Функция генерации ультразвука 40kHz для KY-006
void emitUltrasoundPulse() {
    Serial.println("🔊 Generating 40kHz ultrasound pulse...");

    unsigned long startTime = micros();
    unsigned long endTime = startTime + (PULSE_DURATION * 1000);

    // Генерация 40kHz сигнала (период 25μs)
    while (micros() < endTime) {
        digitalWrite(ULTRASOUND_TX_PIN, HIGH);
        delayMicroseconds(12);
        digitalWrite(ULTRASOUND_TX_PIN, LOW);
        delayMicroseconds(12);
    }

    digitalWrite(ULTRASOUND_TX_PIN, LOW);
    Serial.println("✅ Ultrasound pulse completed");
}

// Функция отправки времени детектирования
void sendDetectionTime() {
    unsigned long detectionTime = micros();
    String message = "BEACON_TIME:" + String(beaconId) + ":" + String(detectionTime);

    IPAddress receiverIP(192, 168, 4, 10);
    udp.beginPacket(receiverIP, UDP_PORT);
    udp.print(message);
    udp.endPacket();

    Serial.println("📤 Sent detection time: " + String(detectionTime) + " μs");

    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(100);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(100);
    digitalWrite(STATUS_LED_PIN, HIGH);
}

// Функция для тестового режима - непрерывное излучение
void testUltrasound() {
    Serial.println("🧪 TEST MODE: Continuous ultrasound transmission");
    Serial.println("⚠️  Press any key in Serial to stop");

    unsigned long testStart = millis();
    int pulseCount = 0;

    while (!Serial.available()) {
        digitalWrite(STATUS_LED_PIN, HIGH);
        emitUltrasoundPulse();
        digitalWrite(STATUS_LED_PIN, LOW);

        pulseCount++;
        if (millis() - testStart > 5000) {
            Serial.println("🔊 Pulses emitted: " + String(pulseCount));
            testStart = millis();
        }

        delay(500);
    }

    while (Serial.available()) Serial.read();
    Serial.println("✅ Test mode stopped");
}

void loop() {
    // ОБРАБОТКА ВХОДЯЩИХ UDP КОМАНД
    int packetSize = udp.parsePacket();
    if (packetSize) {
        char packet[50];
        int len = udp.read(packet, sizeof(packet)-1);
        if (len > 0) {
            packet[len] = 0;
            String message = String(packet);
            String senderIP = udp.remoteIP().toString();

            Serial.println("📨 UDP from " + senderIP + ": " + message);

            if (message == "START") {
                Serial.println("🚀 START command received - beginning transmission");

                int delayTime = BEACON_DELAY * beaconId;
                Serial.println("⏰ Delay: " + String(delayTime) + "ms");
                delay(delayTime);

                Serial.println("🎵 Emitting ultrasound pulse...");
                digitalWrite(STATUS_LED_PIN, LOW);
                emitUltrasoundPulse();
                digitalWrite(STATUS_LED_PIN, HIGH);

                sendDetectionTime();

                Serial.println("✅ Transmission sequence completed");
            }
            else if (message == "TEST") {
                Serial.println("🧪 TEST command received");
                testUltrasound();
            }
            else if (message == "STATUS") {
                Serial.println("📊 STATUS command received");
                String statusMsg = "BEACON_STATUS:" + String(beaconId) + ":KY-006:ONLINE";
                IPAddress receiverIP(192, 168, 4, 10);
                udp.beginPacket(receiverIP, UDP_PORT);
                udp.print(statusMsg);
                udp.endPacket();
                Serial.println("📤 Status sent: " + statusMsg);
            }
        }
    }

    // ОБРАБОТКА SERIAL КОМАНД
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        cmd.toUpperCase();

        if (cmd == "TEST" || cmd == "T") {
            Serial.println("🎵 Manual test transmission");
            digitalWrite(STATUS_LED_PIN, LOW);
            emitUltrasoundPulse();
            digitalWrite(STATUS_LED_PIN, HIGH);
            Serial.println("✅ Manual test completed");
        }
        else if (cmd == "START" || cmd == "S") {
            Serial.println("🚀 Manual start sequence");
            digitalWrite(STATUS_LED_PIN, LOW);
            emitUltrasoundPulse();
            digitalWrite(STATUS_LED_PIN, HIGH);
            sendDetectionTime();
        }
        else if (cmd == "CONTINUOUS" || cmd == "C") {
            testUltrasound();
        }
        else if (cmd == "STATUS" || cmd == "INFO") {
            Serial.println("=== BEACON " + String(beaconId) + " STATUS ===");
            Serial.println("🔧 ID: " + String(beaconId));
            Serial.println("🎵 Type: KY-006 Ultrasound Transmitter");
            Serial.println("📡 IP: " + WiFi.localIP().toString());
            Serial.println("📶 WiFi RSSI: " + String(WiFi.RSSI()) + " dBm");
            Serial.println("🔌 Transmitter Pin: " + String(ULTRASOUND_TX_PIN));
            Serial.println("💡 Status LED Pin: " + String(STATUS_LED_PIN));
            Serial.println("⏰ Pulse Duration: " + String(PULSE_DURATION) + "ms");
            Serial.println("🔄 Uptime: " + String(millis()/1000) + "s");
        }
        else if (cmd == "HELP" || cmd == "H") {
            Serial.println("=== BEACON " + String(beaconId) + " COMMANDS ===");
            Serial.println("TEST/t       - Single ultrasound pulse");
            Serial.println("START/s      - Pulse + send detection time");
            Serial.println("CONTINUOUS/c - Continuous test mode");
            Serial.println("STATUS/info  - System information");
            Serial.println("HELP/h       - This help message");
        }
        else if (cmd == "RESTART" || cmd == "R") {
            Serial.println("🔄 Restarting beacon...");
            ESP.restart();
        }
        else if (cmd.length() > 0) {
            Serial.println("❌ Unknown command: " + cmd);
            Serial.println("💡 Type 'HELP' for available commands");
        }
    }

    // АВТОНОМНЫЙ РЕЖИМ
    static unsigned long lastAutoPulse = 0;
    if (!measurementInProgress && (millis() - lastAutoPulse > 10000)) {
        lastAutoPulse = millis();
        Serial.println("🔊 Auto test pulse");
        digitalWrite(STATUS_LED_PIN, LOW);
        emitUltrasoundPulse();
        digitalWrite(STATUS_LED_PIN, HIGH);
    }

    // ИНДИКАЦИЯ РАБОТЫ
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 2000) {
        digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
        lastBlink = millis();
    }

    delay(50);
}

// ===== РЕАЛИЗАЦИЯ ОБЪЕКТА =====
#elif defined(OBJECT_NODE)

#include <WiFi.h>
#include <WiFiUdp.h>
#include "config/config.h"

WiFiUDP udp;

void setup() {
    Serial.begin(115200);
    pinMode(ULTRASOUND_TX_PIN, OUTPUT);
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(ULTRASOUND_TX_PIN, LOW);
    digitalWrite(STATUS_LED_PIN, LOW);

    Serial.println("Object node initializing...");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        udp.begin(UDP_PORT);
        Serial.println("\nObject node ready!");
        Serial.println("IP: " + WiFi.localIP().toString());
        Serial.println("Type: " + String(SENSOR_TYPE));
        digitalWrite(STATUS_LED_PIN, HIGH);
    } else {
        Serial.println("\nWiFi connection failed!");
    }
}

void emitObjectPulse() {
    for (int i = 0; i < OBJECT_ID; i++) {
        unsigned long startTime = micros();

        while (micros() - startTime < PULSE_DURATION * 1000) {
            digitalWrite(ULTRASOUND_TX_PIN, HIGH);
            delayMicroseconds(12);
            digitalWrite(ULTRASOUND_TX_PIN, LOW);
            delayMicroseconds(12);
        }

        if (i < OBJECT_ID - 1) {
            delay(BETWEEN_PULSE_DELAY);
        }
    }
}

void sendObjectTime() {
    unsigned long currentTime = micros();
    String message = "OBJECT_TIME:" + String(currentTime);

    IPAddress receiverIP(192, 168, 4, 10);
    udp.beginPacket(receiverIP, UDP_PORT);
    udp.print(message);
    udp.endPacket();

    Serial.println("Sent object time: " + String(currentTime) + " μs");
}

void loop() {
    int packetSize = udp.parsePacket();
    if (packetSize) {
        char packet[50];
        int len = udp.read(packet, sizeof(packet)-1);
        if (len > 0) {
            packet[len] = 0;
            String message = String(packet);
            String senderIP = udp.remoteIP().toString();

            Serial.println("Command from " + senderIP + ": " + message);

            if (message == "START") {
                Serial.println("Starting object transmission");
                delay(MEASUREMENT_DELAY);

                digitalWrite(STATUS_LED_PIN, LOW);
                emitObjectPulse();
                digitalWrite(STATUS_LED_PIN, HIGH);

                sendObjectTime();
                Serial.println("Object transmission completed");
            }
        }
    }

    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        if (cmd == "test" || cmd == "t") {
            Serial.println("Manual object test");
            digitalWrite(STATUS_LED_PIN, LOW);
            emitObjectPulse();
            digitalWrite(STATUS_LED_PIN, HIGH);
        }
        else if (cmd == "status" || cmd == "s") {
            Serial.println("=== OBJECT STATUS ===");
            Serial.println("Type: " + String(SENSOR_TYPE));
            Serial.println("IP: " + WiFi.localIP().toString());
            Serial.println("Pulse pattern: " + String(OBJECT_ID) + " pulses");
            Serial.println("WiFi RSSI: " + String(WiFi.RSSI()) + " dBm");
        }
    }

    static unsigned long lastAutoPulse = 0;
    if (millis() - lastAutoPulse > OBJECT_PULSE_INTERVAL) {
        lastAutoPulse = millis();
        Serial.println("Auto object pulse");
        digitalWrite(STATUS_LED_PIN, LOW);
        emitObjectPulse();
        digitalWrite(STATUS_LED_PIN, HIGH);
    }

    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 1000) {
        digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
        lastBlink = millis();
    }

    delay(100);
}

#else
#error "Please define RECEIVER_NODE, BEACON_NODE or OBJECT_NODE in build flags"
#endif
