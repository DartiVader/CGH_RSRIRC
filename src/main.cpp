#include <Arduino.h>

// ===== РЕАЛИЗАЦИЯ РЕСИВЕРА =====
#ifdef RECEIVER_NODE

#include <WiFi.h>
#include <WiFiUdp.h>
#include "config/config.h"
#include "positioning/positioning.h"
#include "network/wifi_manager.h"
#include "hardware/ultrasound.h"
#include "WIFI_App/WIFI_App.h"
#include "web_server/web_server.h"

PositioningSystem positioning;
WiFiManager wifi;
Ultrasound ultrasound;
WIFI_App wifiApp;
WebService webService;

// Переменные для TDOA
unsigned long pulseArrivalTimes[3] = {0};
bool pulseDetected[3] = {false};
unsigned long measurementStartTime = 0;
bool measurementCycleActive = false;

// Переменные для управления объектом
unsigned long lastObjectCommand = 0;
bool objectSoundEnabled = false;
int measurementCount = 0;

void calculatePosition();

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n=== 🚀 СИСТЕМА ПОЗИЦИОНИРОВАНИЯ - ПРИЕМНИК ===");
    Serial.println("Режим: TDOA с веб-интерфейсом");
    Serial.println("Версия: с управлением объектом");

    // 1. Инициализация WiFi AP
    Serial.println("📡 Запуск точки доступа WiFi...");
    wifiApp.begin();

    // 2. Инициализация веб-сервера
    Serial.println("🌐 Запуск веб-сервера...");
    webService.begin();

    // 3. Инициализация компонентов позиционирования
    Serial.println("🎯 Инициализация системы позиционирования...");
    positioning.begin();

    Serial.println("📡 Запуск UDP...");
    wifi.startUDP();

    Serial.println("🎤 Настройка ультразвукового приемника...");
    ultrasound.setupReceiver();

    // Настройка пинов
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, HIGH);

    Serial.println("✅ Инициализация приемника завершена!");
    Serial.println("📶 Подключитесь к WiFi: " + String(WIFI_SSID));
    Serial.println("🌐 Откройте в браузере: http://" + WiFi.softAPIP().toString());
    Serial.println("🎯 Система готова к измерениям");
}

void startMeasurementCycle() {
    measurementCount++;
    measurementCycleActive = true;
    measurementStartTime = millis();

    Serial.println("\n=== 🔄 ЦИКЛ ИЗМЕРЕНИЯ #" + String(measurementCount) + " ===");
    Serial.println("🔄 ОТПРАВКА КОМАНД МАЯКАМ И ОБЪЕКТУ...");

    // Сбрасываем таймеры ожидания
    memset(pulseDetected, 0, sizeof(pulseDetected));
    pulseArrivalTimes[0] = micros();

    // Команда маякам
    wifi.sendUDPBroadcast("START");
    Serial.println("📡 Команда START отправлена маякам");

    // Запланировать команду объекту через 500ms
    objectSoundEnabled = true;
    lastObjectCommand = millis();

    Serial.println("⏳ Ожидание 500ms перед командой объекту...");
}

void processBeaconTime(int beaconId, unsigned long beaconTime) {
    pulseArrivalTimes[beaconId] = beaconTime;
    pulseDetected[beaconId] = true;

    Serial.println("📊 Маяк " + String(beaconId) + " время: " + String(beaconTime) + " мкс");

    // Проверяем готовность TDOA расчета
    if (pulseDetected[0] && pulseDetected[1] && pulseDetected[2]) {
        Serial.println("🎯 ВСЕ ДАННЫЕ ПОЛУЧЕНЫ - ВОЗМОЖЕН РАСЧЕТ TDOA!");
        calculatePosition();
    }
}

void calculatePosition() {
    Serial.println("\n=== 🧮 РАСЧЕТ ПОЗИЦИИ ===");

    // Выводим временные метки
    Serial.println("   Приемник: " + String(pulseArrivalTimes[0]) + " мкс");
    Serial.println("   Маяк 1: " + String(pulseArrivalTimes[1]) + " мкс");
    Serial.println("   Маяк 2: " + String(pulseArrivalTimes[2]) + " мкс");

    // Рассчитываем разницы времени
    long diff1 = pulseArrivalTimes[1] - pulseArrivalTimes[0];
    long diff2 = pulseArrivalTimes[2] - pulseArrivalTimes[0];

    Serial.println("   Разницы времени: " + String(diff1) + " мкс, " + String(diff2) + " мкс");

    // Преобразуем время в расстояния (скорость звука 34300 см/с)
    float distance1 = (diff1 / 1000000.0) * 34300.0;
    float distance2 = (diff2 / 1000000.0) * 34300.0;

    Serial.println("   Расстояния: " + String(distance1) + " см, " + String(distance2) + " см");

    // Используем систему позиционирования для расчета
    float distances[3] = {0, distance1, distance2};
    Position pos = positioning.trilaterate(distances);

    // Конвертируем в структуру для веб-сервиса
    PositionData webPos;
    webPos.x = pos.x;
    webPos.y = pos.y;
    webPos.accuracy = pos.accuracy;
    webPos.timestamp = millis();
    webPos.valid = (pos.accuracy < 50.0);

    // Обновляем веб-сервис
    webService.updatePosition(webPos);

    Serial.printf("📍 Позиция: X=%.1fсм, Y=%.1fсм, Точность=%.1fсм\n",
                 pos.x, pos.y, pos.accuracy);

    // Завершаем цикл измерений
    measurementCycleActive = false;
    memset(pulseDetected, 0, sizeof(pulseDetected));

    Serial.println("✅ Цикл измерений завершен");
}

void loop() {
    // Мигание светодиодом
    static unsigned long lastLedToggle = 0;
    if (millis() - lastLedToggle > 1000) {
        digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
        lastLedToggle = millis();
    }

    // Автоматическая отправка START команды если измерения активны в веб-интерфейсе
    static unsigned long lastStartCommand = 0;
    if (webService.isMeasuring() && millis() - lastStartCommand > 2000) {
        lastStartCommand = millis();
        startMeasurementCycle();
    }

    // Управление объектом - отправка звуковой команды после задержки
    if (objectSoundEnabled && (millis() - lastObjectCommand > 500)) {
        objectSoundEnabled = false;
        Serial.println("🔊 ОТПРАВКА КОМАНДЫ ЗВУКА ОБЪЕКТУ");
        wifi.sendUDPBroadcast("SOUND_ON");
    }

    // Чтение значения сенсора приемника
    static unsigned long lastSensorRead = 0;
    if (millis() - lastSensorRead > 50) {
        lastSensorRead = millis();

        if (ultrasound.detectPulse()) {
            unsigned long arrivalTime = micros();
            pulseArrivalTimes[0] = arrivalTime;
            pulseDetected[0] = true;

            Serial.println("🎯 ИМПУЛЬС ОБНАРУЖЕН! Время: " + String(arrivalTime) + " мкс");

            // Отправляем запрос времени маякам
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
            String senderIP = wifi.udp.remoteIP().toString();

            // Обработка времени от маяков
            if (message.startsWith("BEACON_TIME:")) {
                int beaconId = message.substring(12, 13).toInt();
                unsigned long beaconTime = message.substring(14).toInt();
                processBeaconTime(beaconId, beaconTime);
            }
            // Обработка подтверждения от объекта
            else if (message.startsWith("OBJECT_SOUND:")) {
                Serial.println("✅ Подтверждение от объекта: " + message);
            }
            // Обработка статуса от объектов
            else if (message.startsWith("OBJECT_STATUS:")) {
                Serial.println("📊 Статус объекта: " + message);
            }
            else {
                Serial.println("📨 Сообщение от " + senderIP + ": " + message);
            }
        }
    }

    // Обновление веб-интерфейса
    static unsigned long lastPositionUpdate = 0;
    if (millis() - lastPositionUpdate > 1000) {
        lastPositionUpdate = millis();

        positioning.update();
        Position pos = positioning.getCurrentPosition();

        PositionData webPos;
        webPos.x = pos.x;
        webPos.y = pos.y;
        webPos.accuracy = pos.accuracy;
        webPos.timestamp = millis();
        webPos.valid = (pos.accuracy < 50.0);

        webService.updatePosition(webPos);
    }

    // Диагностика каждые 30 секунд
    static unsigned long lastDiagnostic = 0;
    if (millis() - lastDiagnostic > 30000) {
        lastDiagnostic = millis();
        Serial.println("\n=== 🔍 ДИАГНОСТИКА ПРИЕМНИКА ===");
        Serial.println("   WiFi клиентов: " + String(WiFi.softAPgetStationNum()));
        Serial.println("   IP: " + WiFi.softAPIP().toString());
        Serial.println("   Циклов измерений: " + String(measurementCount));
        Serial.println("   Активен цикл: " + String(measurementCycleActive ? "Да" : "Нет"));
        Serial.println("   Измерения в веб-интерфейсе: " + String(webService.isMeasuring() ? "Вкл" : "Выкл"));
        Serial.println("   Память: " + String(esp_get_free_heap_size()) + " байт");
    }

    delay(50);
}

// ===== РЕАЛИЗАЦИЯ МАЯКА =====
#elif BEACON_NODE

#include "config/config.h"
#include "network/wifi_manager.h"
#include "hardware/ultrasound.h"

WiFiManager wifi;
Ultrasound ultrasound;

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

void testSpeaker() {
    Serial.println("🎵 Testing speaker...");

    #ifdef BEACON_NODE
    #if BEACON_ID == 1
    Serial.println("   KY-006 Passive Buzzer Test");
    for(int i = 0; i < 3; i++) {
        digitalWrite(STATUS_LED_PIN, HIGH);
        Serial.println("   Beep " + String(i+1) + " - Generating 40kHz");

        unsigned long startTime = micros();
        while (micros() - startTime < 200000) {
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
    for(int i = 0; i < 3; i++) {
        digitalWrite(STATUS_LED_PIN, HIGH);
        Serial.println("   Beep " + String(i+1) + " - PWM 40kHz");

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

void emitSimplePulse() {
    Serial.println("🔊 EMITTING ULTRASOUND PULSE");

    #ifdef BEACON_NODE
    #if BEACON_ID == 1
    Serial.println("   KY-006: Software 40kHz generation");
    unsigned long startTime = micros();
    long cycleCount = 0;

    while (micros() - startTime < 100000) {
        digitalWrite(ULTRASOUND_TX_PIN, HIGH);
        delayMicroseconds(12);
        digitalWrite(ULTRASOUND_TX_PIN, LOW);
        delayMicroseconds(12);
        cycleCount++;
    }
    Serial.println("   Generated " + String(cycleCount) + " cycles");

    #elif BEACON_ID == 2
    Serial.println("   KY-012: PWM 40kHz");
    ledcWrite(0, 127);
    delay(100);
    ledcWrite(0, 0);
    Serial.println("   PWM tone completed");
    #endif
    #endif
}

void loop() {
    static unsigned long lastLedToggle = 0;
    if (millis() - lastLedToggle > 1000) {
        digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
        lastLedToggle = millis();
    }

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

                for(int i = 0; i < 5; i++) {
                    digitalWrite(STATUS_LED_PIN, HIGH);
                    delay(100);
                    digitalWrite(STATUS_LED_PIN, LOW);
                    delay(100);
                }

                int delayTime = 100 + (BEACON_ID * 100);
                Serial.println("⏳ Waiting " + String(delayTime) + "ms before sound...");
                delay(delayTime);

                Serial.println("🔊 EMITTING ULTRASOUND PULSE...");
                emitSimplePulse();

                Serial.println("✅ Ultrasound emission completed");

                unsigned long currentTime = micros();
                String ackMessage = "BEACON_TIME:" + String(BEACON_ID) + ":" + String(currentTime);
                wifi.sendUDPBroadcast(ackMessage);
                Serial.println("📤 Sent time: " + ackMessage);

            } else {
                Serial.println("❓ Unknown command: " + message);
            }
        }
    }

    static unsigned long lastDiagnostic = 0;
    if (millis() - lastDiagnostic > 10000) {
        lastDiagnostic = millis();
        Serial.println("=== 🔍 BEACON DIAGNOSTIC ===");
        Serial.println("   WiFi: " + String(WiFi.SSID()) + " (" + String(WiFi.RSSI()) + "dBm)");
        Serial.println("   IP: " + WiFi.localIP().toString());
        Serial.println("   Status: Waiting for START command");
        Serial.println("   Beacon ID: " + String(BEACON_ID));
    }

    static unsigned long lastWifiCheck = 0;
    if (millis() - lastWifiCheck > 10000) {
        lastWifiCheck = millis();
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("🔄 WiFi reconnecting...");
            wifi.setupSTA();
        }
    }

    delay(100);
}

// ===== РЕАЛИЗАЦИЯ ОБЪЕКТА =====
#elif OBJECT_NODE

#include <WiFi.h>
#include <WiFiUdp.h>

#define SOUND_PIN 25
#define STATUS_LED_PIN 2
#define UDP_PORT 1234
#define PULSE_DURATION 15
#define OBJECT_ID 3
#define BETWEEN_PULSE_DELAY 10

WiFiUDP udp;
unsigned long lastSoundTime = 0;
int soundCount = 0;

void setupWiFi() {
  Serial.println("📡 Подключение к WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi подключен!");
    Serial.print("📶 IP: ");
    Serial.println(WiFi.localIP());
    digitalWrite(STATUS_LED_PIN, HIGH);
  } else {
    Serial.println("\n❌ Ошибка подключения к WiFi!");
    digitalWrite(STATUS_LED_PIN, LOW);
  }
}

void emitSinglePulse() {
  unsigned long startTime = micros();

  while (micros() - startTime < PULSE_DURATION * 1000) {
    digitalWrite(SOUND_PIN, HIGH);
    delayMicroseconds(12);
    digitalWrite(SOUND_PIN, LOW);
    delayMicroseconds(12);
  }
}

void emitCodedSound() {
  soundCount++;
  Serial.println("🎵 ИСПУСКАЮ КОДИРОВАННЫЙ ЗВУК - ID: " + String(OBJECT_ID));
  digitalWrite(STATUS_LED_PIN, HIGH);

  for (int i = 0; i < OBJECT_ID; i++) {
    Serial.println("   🔊 Импульс " + String(i+1) + "/" + String(OBJECT_ID));
    emitSinglePulse();

    if (i < OBJECT_ID - 1) {
      delay(BETWEEN_PULSE_DELAY);
    }
  }

  digitalWrite(STATUS_LED_PIN, LOW);
  lastSoundTime = millis();
  Serial.println("✅ Звуковой сигнал завершен");
}

void sendSoundConfirmation() {
  String ack = "OBJECT_SOUND:" + String(millis()) + ":COUNT:" + String(soundCount);

  IPAddress broadcastIP(192, 168, 4, 255);
  udp.beginPacket(broadcastIP, UDP_PORT);
  udp.print(ack);
  udp.endPacket();

  Serial.println("📤 Отправлено подтверждение: " + ack);
}

void setup() {
  Serial.begin(115200);
  pinMode(SOUND_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(SOUND_PIN, LOW);
  digitalWrite(STATUS_LED_PIN, LOW);

  delay(1000);

  Serial.println("\n=== 🎯 ESP32 ОБЪЕКТ СО ЗВУКОМ ===");
  Serial.println("🔊 Прямое управление звуком: GPIO25 → BC547 → PAM8403");
  Serial.println("📍 Идентификатор: " + String(OBJECT_ID) + " импульсов");
  Serial.println("📡 Сеть: " + String(WIFI_SSID));

  setupWiFi();
  udp.begin(UDP_PORT);

  Serial.println("\n🔊 ТЕСТ ПРИ ЗАПУСКЕ...");
  emitCodedSound();
  sendSoundConfirmation();

  Serial.println("\n✅ Объект инициализирован!");
  Serial.println("🎯 Ожидание UDP команд на порту " + String(UDP_PORT));
}

void loop() {
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 1000) {
    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
    lastBlink = millis();
  }

  int packetSize = udp.parsePacket();
  if (packetSize) {
    char packet[100];
    int len = udp.read(packet, sizeof(packet) - 1);
    if (len > 0) {
      packet[len] = 0;
      String message = String(packet);
      String senderIP = udp.remoteIP().toString();

      Serial.println("\n📨 UDP ОТ " + senderIP + ": " + message);

      if (message == "START" || message == "SOUND_ON" || message == "SOUND") {
        Serial.println("🚀 КОМАНДА ЗВУКА ПОЛУЧЕНА - ИСПУСКАЮ ЗВУК");
        emitCodedSound();
        sendSoundConfirmation();
      }
      else if (message == "STATUS") {
        Serial.println("📊 ЗАПРОС СТАТУСА");
        String status = "OBJECT_STATUS:ID=" + String(OBJECT_ID) +
                       ",IP=" + WiFi.localIP().toString() +
                       ",RSSI=" + String(WiFi.RSSI()) +
                       ",COUNT=" + String(soundCount);
        udp.beginPacket(udp.remoteIP(), UDP_PORT);
        udp.print(status);
        udp.endPacket();
        Serial.println("📤 Отправлен статус: " + status);
      }
    }
  }

  static unsigned long lastDiagnostic = 0;
  if (millis() - lastDiagnostic > 30000) {
    lastDiagnostic = millis();
    Serial.println("=== 🔍 ДИАГНОСТИКА ОБЪЕКТА ===");
    Serial.println("   WiFi: " + String(WiFi.SSID()) + " (" + String(WiFi.RSSI()) + "dBm)");
    Serial.println("   IP: " + WiFi.localIP().toString());
    Serial.println("   Всего сигналов: " + String(soundCount));
    Serial.println("   Последний сигнал: " + String((millis() - lastSoundTime) / 1000) + " сек назад");
  }

  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command == "TEST" || command == "T") {
      Serial.println("🔊 РУЧНОЙ ТЕСТ");
      emitCodedSound();
      sendSoundConfirmation();
    }
    else if (command == "STATUS" || command == "S") {
      Serial.println("=== СТАТУС ОБЪЕКТА ===");
      Serial.println("Пин звука: GPIO" + String(SOUND_PIN));
      Serial.println("ID объекта: " + String(OBJECT_ID) + " импульсов");
      Serial.println("WiFi: " + String(WiFi.SSID()));
      Serial.println("Сигналов отправлено: " + String(soundCount));
    }
    else if (command == "WIFI" || command == "W") {
      Serial.println("🔄 Переподключение WiFi...");
      setupWiFi();
    }
  }

  delay(100);
}

#else
#error "Please define either RECEIVER_NODE, BEACON_NODE or OBJECT_NODE in build flags"
#endif
