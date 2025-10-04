#include <Arduino.h>
#include "config/config.h"
#include "positioning/positioning.h"
#include "network/wifi_manager.h"
#include "hardware/ultrasound.h"
#include "WIFI_App/WIFI_App.h"

#include "web_server/web_server.h"
// ===== РЕАЛИЗАЦИЯ РЕСИВЕРА =====
#ifdef RECEIVER_NODE

PositioningSystem positioning;
WiFiManager wifi;
Ultrasound ultrasound;
WIFI_App wifiApp;
WebService webService;

// Переменные для TDOA
unsigned long pulseArrivalTimes[3] = {0};
bool pulseDetected[3] = {false};

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n=== 🚀 POSITIONING SYSTEM - RECEIVER ===");
    Serial.println("Mode: TDOA with Web Interface");

    // 1. Инициализация WiFi AP
    Serial.println("📡 Starting WiFi Access Point...");
    wifiApp.begin();

    // 2. Инициализация веб-сервера
    Serial.println("🌐 Starting Web Server...");
    webService.begin();

    // 3. Инициализация компонентов позиционирования
    positioning.begin();
    wifi.startUDP();
    ultrasound.setupReceiver();

    // Настройка пинов
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, HIGH);

    Serial.println("✅ Receiver initialization completed!");
    Serial.println("📶 Connect to WiFi: " + String(WIFI_SSID));
    Serial.println("🌐 Open in browser: http://" + WiFi.softAPIP().toString());
}

void loop() {
    // Мигание светодиодом
    static unsigned long lastLedToggle = 0;
    if (millis() - lastLedToggle > 1000) {
        digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
        lastLedToggle = millis();
    }

    // Автоматическая отправка START команды если измерения активны
    static unsigned long lastStartCommand = 0;
    if (webService.isMeasuring() && millis() - lastStartCommand > 2000) {
        lastStartCommand = millis();

        Serial.println("🔄 SENDING START COMMAND TO BEACONS...");
        wifi.sendUDPBroadcast("START");

        // Сбрасываем таймеры ожидания
        memset(pulseDetected, 0, sizeof(pulseDetected));
        pulseArrivalTimes[0] = micros();
    }

    // Чтение значения сенсора
    static unsigned long lastSensorRead = 0;
    if (millis() - lastSensorRead > 100) {
        lastSensorRead = millis();

        if (ultrasound.detectPulse()) {
            unsigned long arrivalTime = micros();
            pulseArrivalTimes[0] = arrivalTime;
            pulseDetected[0] = true;

            Serial.println("🎯 PULSE DETECTED! Time: " + String(arrivalTime));
            wifi.sendUDPBroadcast("REQUEST_TIME:" + String(arrivalTime));
        }
    }

    // Обработка входящих UDP сообщений
    int packetSize = wifi.udp.parsePacket();
    if (packetSize) {
        char packet[150];
        int len = wifi.udp.read(packet, 150);
        if (len > 0) {
            packet[len] = 0;
            String message = String(packet);

            if (message.startsWith("BEACON_TIME:")) {
                int beaconId = message.substring(12, 13).toInt();
                unsigned long beaconTime = message.substring(14).toInt();

                pulseArrivalTimes[beaconId] = beaconTime;
                pulseDetected[beaconId] = true;

                Serial.println("📊 Beacon " + String(beaconId) + " time: " + String(beaconTime));

                // Проверяем TDOA
                if (pulseDetected[0] && pulseDetected[1] && pulseDetected[2]) {
                    Serial.println("🎯 TDOA CALCULATION POSSIBLE!");
                    memset(pulseDetected, 0, sizeof(pulseDetected));
                }
            }
        }
    }

    // Обновление позиции и веб-интерфейса
    static unsigned long lastPositionUpdate = 0;
    if (millis() - lastPositionUpdate > 500) {
        lastPositionUpdate = millis();

        positioning.update();
        Position pos = positioning.getCurrentPosition();

        // Конвертируем в структуру для веб-сервиса
        PositionData webPos;
        webPos.x = pos.x;
        webPos.y = pos.y;
        webPos.accuracy = pos.accuracy;
        webPos.timestamp = millis();
        webPos.valid = (pos.accuracy < 50.0);

        // Обновляем веб-сервис
        webService.updatePosition(webPos);

        Serial.printf("📍 Position: X=%.1fcm, Y=%.1fcm, Accuracy=%.1fcm\n",
                     pos.x, pos.y, pos.accuracy);
    }

    delay(50);
}

void checkTDOA() {
    if (pulseDetected[0] && pulseDetected[1] && pulseDetected[2]) {
        Serial.println("🎯 TDOA CALCULATION POSSIBLE!");
        Serial.println("   Receiver: " + String(pulseArrivalTimes[0]));
        Serial.println("   Beacon 1: " + String(pulseArrivalTimes[1]));
        Serial.println("   Beacon 2: " + String(pulseArrivalTimes[2]));

        // Рассчитываем разницы времени
        long diff1 = pulseArrivalTimes[1] - pulseArrivalTimes[0];
        long diff2 = pulseArrivalTimes[2] - pulseArrivalTimes[0];

        Serial.println("   Time differences: " + String(diff1) + "us, " + String(diff2) + "us");

        // Здесь будет расчет позиции на основе TDOA
        // Пока используем заглушку из positioning system

        // Сбрасываем флаги
        memset(pulseDetected, 0, sizeof(pulseDetected));
    }
}

// ===== РЕАЛИЗАЦИЯ МАЯКА =====
#elif BEACON_NODE

WiFiManager wifi;
Ultrasound ultrasound;

// ОБЪЯВЛЯЕМ ФУНКЦИИ ДО ИХ ИСПОЛЬЗОВАНИЯ
void testSpeaker();
void emitSimplePulse();

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

    // Тестируем динамик сразу при запуске
    Serial.println("🔊 TEST: Initial speaker test...");
    testSpeaker();

    // Настройка пинов
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, HIGH);

    Serial.println("✅ Beacon initialization completed!");
}

// Функция тестирования динамика
void testSpeaker() {
    Serial.println("🎵 Testing speaker...");

    #ifdef BEACON_NODE
    #if BEACON_ID == 1
    Serial.println("   KY-006 Passive Buzzer Test");
    // Тест для KY-006 - генерируем тон программно
    for(int i = 0; i < 3; i++) {
        digitalWrite(STATUS_LED_PIN, HIGH);
        Serial.println("   Beep " + String(i+1) + " - Generating 40kHz");

        // Генерируем 40kHz на 200ms
        unsigned long startTime = micros();
        while (micros() - startTime < 200000) { // 200ms
            digitalWrite(ULTRASOUND_TX_PIN, HIGH);
            delayMicroseconds(12);
            digitalWrite(ULTRASOUND_TX_PIN, LOW);
            delayMicroseconds(12);
        }

        digitalWrite(STATUS_LED_PIN, LOW);
        delay(200);
    }

    #elif BEACON_ID == 2
    Serial.println("   KY-012 Active Buzzer Test");
    // Тест для KY-012 - используем ШИМ
    for(int i = 0; i < 3; i++) {
        digitalWrite(STATUS_LED_PIN, HIGH);
        Serial.println("   Beep " + String(i+1) + " - PWM 40kHz");

        // Включаем ШИМ на 200ms
        ledcWrite(0, 127);
        delay(200);
        ledcWrite(0, 0);

        digitalWrite(STATUS_LED_PIN, LOW);
        delay(200);
    }
    #endif
    #endif

    Serial.println("✅ Speaker test completed");
}

// Простая функция излучения ультразвука
void emitSimplePulse() {
    Serial.println("🔊 EMITTING ULTRASOUND PULSE");

    #ifdef BEACON_NODE
    #if BEACON_ID == 1
    // KY-006 - пассивный зуммер
    Serial.println("   KY-006: Software 40kHz generation");
    unsigned long startTime = micros();
    long cycleCount = 0;

    while (micros() - startTime < 100000) { // 100ms
        digitalWrite(ULTRASOUND_TX_PIN, HIGH);
        delayMicroseconds(12);
        digitalWrite(ULTRASOUND_TX_PIN, LOW);
        delayMicroseconds(12);
        cycleCount++;
    }
    Serial.println("   Generated " + String(cycleCount) + " cycles");

    #elif BEACON_ID == 2
    // KY-012 - активный зуммер
    Serial.println("   KY-012: PWM 40kHz");
    ledcWrite(0, 127); // Включаем ШИМ
    delay(100); // 100ms
    ledcWrite(0, 0);   // Выключаем ШИМ
    Serial.println("   PWM tone completed");
    #endif
    #endif
}

void loop() {
    // Медленное мигание светодиодом в режиме ожидания
    static unsigned long lastLedToggle = 0;
    if (millis() - lastLedToggle > 1000) {
        digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
        lastLedToggle = millis();
    }

    // Проверяем входящие UDP сообщения
    int packetSize = wifi.udp.parsePacket();
    if (packetSize) {
        char packet[100];
        int len = wifi.udp.read(packet, 100);
        if (len > 0) {
            packet[len] = 0;
            String message = String(packet);
            String senderIP = wifi.udp.remoteIP().toString();

            Serial.println("=== 📨 UDP MESSAGE RECEIVED ===");
            Serial.println("   From: " + senderIP);
            Serial.println("   Message: " + message);
            Serial.println("   Length: " + String(len));

            if (message == "START") {
                Serial.println("🚀 START COMMAND RECEIVED - EMITTING SOUND!");

                // Быстрое мигание для визуального подтверждения
                for(int i = 0; i < 5; i++) {
                    digitalWrite(STATUS_LED_PIN, HIGH);
                    delay(100);
                    digitalWrite(STATUS_LED_PIN, LOW);
                    delay(100);
                }

                // Задержка для предотвращения интерференции
                int delayTime = 100 + (BEACON_ID * 100); // Разная задержка для каждого маяка
                Serial.println("⏳ Waiting " + String(delayTime) + "ms before sound...");
                delay(delayTime);

                // ИЗДАЕМ ЗВУК - используем простой метод сначала
                Serial.println("🔊 EMITTING ULTRASOUND PULSE...");

                // Способ 1: Простой ШИМ сигнал (более надежный)
                emitSimplePulse();

                Serial.println("✅ Ultrasound emission completed");

                // Отправляем подтверждение
                unsigned long currentTime = micros();
                String ackMessage = "BEACON_TIME:" + String(BEACON_ID) + ":" + String(currentTime);
                wifi.sendUDPBroadcast(ackMessage);
                Serial.println("📤 Sent time: " + ackMessage);

            } else {
                Serial.println("❓ Unknown command: " + message);
            }
        }
    }

    // Диагностика каждые 10 секунд
    static unsigned long lastDiagnostic = 0;
    if (millis() - lastDiagnostic > 10000) {
        lastDiagnostic = millis();
        Serial.println("=== 🔍 BEACON DIAGNOSTIC ===");
        Serial.println("   WiFi: " + String(WiFi.SSID()) + " (" + String(WiFi.RSSI()) + "dBm)");
        Serial.println("   IP: " + WiFi.localIP().toString());
        Serial.println("   Status: Waiting for START command");
        Serial.println("   Beacon ID: " + String(BEACON_ID));
    }

    // Проверка WiFi
    static unsigned long lastWifiCheck = 0;
    if (millis() - lastWifiCheck > 10000) {
        lastWifiCheck = millis();
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("🔄 WiFi reconnecting...");
            wifi.setupSTA();
        } else {
            Serial.println("📶 WiFi: " + String(WiFi.SSID()) + " (" + String(WiFi.RSSI()) + "dBm)");
        }
    }

    delay(100);
}

// ===== РЕАЛИЗАЦИЯ ОБЪЕКТА =====
#elif OBJECT_NODE

// Объявляем экземпляры классов для объекта
WiFiManager wifi;
Ultrasound ultrasound;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n=== 🎯 POSITIONING SYSTEM - OBJECT ===");
    Serial.println("Node: " + String(NODE_TYPE));
    Serial.println("Sensor: " + String(SENSOR_TYPE));

    Serial.println("Setting up ultrasound transmitter with PWM...");
    ultrasound.setupTransmitter(); // Теперь включает настройку ШИМ

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

    Serial.println("✅ Object initialization completed!");
    Serial.println("📡 Connected to: " + String(WiFi.SSID()));
    Serial.println("📶 IP address: " + WiFi.localIP().toString());
    Serial.println("🔊 Ready to emit ultrasound pulses every " + String(OBJECT_PULSE_INTERVAL/1000000) + " seconds");
}

void loop() {
    // Быстрое мигание светодиодом (индикация активности)
    static unsigned long lastLedToggle = 0;
    if (millis() - lastLedToggle > 100) {
        digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
        lastLedToggle = millis();
    }

    // Периодическое излучение ультразвуковых импульсов
    static unsigned long lastPulseTime = 0;
    if (micros() - lastPulseTime > OBJECT_PULSE_INTERVAL) {
        lastPulseTime = micros();

        Serial.println("🚀 EMITTING CODED ULTRASOUND PULSE...");

        // ИСПОЛЬЗУЕМ НОВУЮ ФУНКЦИЮ с кодированными импульсами
        ultrasound.emitCodedPulse(255); // 255 = ID объекта

        // Отправляем сообщение о излучении
        String pulseMessage = "OBJECT_PULSE:" + String(micros());
        wifi.sendUDPBroadcast(pulseMessage);
        Serial.println("📤 Sent: " + pulseMessage);

        // Мигаем светодиодом для визуального подтверждения
        digitalWrite(STATUS_LED_PIN, HIGH);
        delay(50);
        digitalWrite(STATUS_LED_PIN, LOW);
    }

    // Проверяем входящие UDP сообщения (для синхронизации)
    int packetSize = wifi.udp.parsePacket();
    if (packetSize) {
        char packet[50];
        int len = wifi.udp.read(packet, 50);
        if (len > 0) {
            packet[len] = 0;
            String message = String(packet);

            Serial.println("📨 Received: " + message);

            if (message == "SYNC") {
                Serial.println("🕒 SYNC command received - emitting immediate pulse");
                ultrasound.emitPulse(255);
            }
        }
    }

    // Проверка WiFi каждые 30 секунд
    static unsigned long lastWifiCheck = 0;
    if (millis() - lastWifiCheck > 30000) {
        lastWifiCheck = millis();
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("🔄 WiFi reconnecting...");
            wifi.setupSTA();
        } else {
            Serial.println("📶 WiFi: " + String(WiFi.SSID()) + " (" + String(WiFi.RSSI()) + "dBm)");
        }
    }

    delay(10);
}
#else
#error "Please define either RECEIVER_NODE or BEACON_NODE in build flags"
#endif
