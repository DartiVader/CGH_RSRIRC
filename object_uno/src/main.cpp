#include <Arduino.h>

#define ULTRASONIC_PIN 8        // Pin 8 → PAM8403 L
#define ESP_TRIGGER_PIN 2       // Пин для команды от ESP32
#define STATUS_LED_PIN 13       // Встроенный светодиод

// Параметры ультразвука
const int PULSE_DURATION = 15;    // Длительность импульса в мс
const int OBJECT_ID_PULSES = 3;   // 3 импульса для идентификации

bool espCommandActive = false;
unsigned long lastCommandTime = 0;

// 🔥 ДОБАВЬТЕ ПРОТОТИПЫ ФУНКЦИЙ ПЕРЕД setup()
void testUltrasound();
void emitSinglePulse();
void emitCodedPulse();
void checkEspCommand();

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(ULTRASONIC_PIN, OUTPUT);
  pinMode(ESP_TRIGGER_PIN, INPUT);

  digitalWrite(ULTRASONIC_PIN, LOW);

  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== 🎯 ARDUINO UNO - ULTRASOUND GENERATOR ===");
  Serial.println("🔊 Amplifier: PAM8403");
  Serial.println("📌 Ultrasound Pin: " + String(ULTRASONIC_PIN));
  Serial.println("🤖 ESP32 Trigger Pin: " + String(ESP_TRIGGER_PIN));
  Serial.println("📍 Object ID: " + String(OBJECT_ID_PULSES) + " pulses");

  // Тестовый сигнал
  testUltrasound();
  Serial.println("\n✅ Arduino ready! Waiting for ESP32 commands...");
}

void testUltrasound() {
  Serial.println("🎵 Testing ultrasound generator...");
  digitalWrite(STATUS_LED_PIN, HIGH);

  for(int i = 0; i < 2; i++) {
    Serial.println("   Pulse " + String(i+1));
    emitSinglePulse();
    delay(200);
  }

  digitalWrite(STATUS_LED_PIN, LOW);
  Serial.println("✅ Ultrasound test completed");
}

void emitSinglePulse() {
  // Генерируем 40kHz сигнал
  unsigned long startTime = micros();
  while (micros() - startTime < PULSE_DURATION * 1000) {
    digitalWrite(ULTRASONIC_PIN, HIGH);
    delayMicroseconds(12); // Полупериод 40kHz
    digitalWrite(ULTRASONIC_PIN, LOW);
    delayMicroseconds(12);
  }
}

void emitCodedPulse() {
  Serial.println("🚀 EMITTING CODED ULTRASOUND");
  digitalWrite(STATUS_LED_PIN, HIGH);

  for (int i = 0; i < OBJECT_ID_PULSES; i++) {
    Serial.println("   🔊 Pulse " + String(i+1) + "/" + String(OBJECT_ID_PULSES));
    emitSinglePulse();

    if (i < OBJECT_ID_PULSES - 1) {
      delay(10); // Пауза между импульсами
    }
  }

  digitalWrite(STATUS_LED_PIN, LOW);
  Serial.println("✅ Coded pulse completed");
}

void checkEspCommand() {
  // Проверяем команду от ESP32
  if (digitalRead(ESP_TRIGGER_PIN) == HIGH) {
    if (!espCommandActive) {
      espCommandActive = true;
      lastCommandTime = millis();
      Serial.println("🎯 ESP32 COMMAND RECEIVED!");
      emitCodedPulse();
    }
  } else {
    // Сбрасываем флаг через 100ms после команды
    if (espCommandActive && (millis() - lastCommandTime > 100)) {
      espCommandActive = false;
    }
  }
}

void loop() {
  // Проверка команды от ESP32
  checkEspCommand();

  // Мигание светодиодом в режиме ожидания
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 1000 && !espCommandActive) {
    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
    lastBlink = millis();
  }

  // Автономная работа (если ESP32 не управляет)
  static unsigned long lastAutoPulse = 0;
  if (millis() - lastAutoPulse > 5000 && !espCommandActive) { // Каждые 5 секунд
    lastAutoPulse = millis();
    Serial.println("🔄 AUTO: Emitting pulse");
    emitCodedPulse();
  }

  // Обработка Serial команд
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command == "TEST" || command == "t") {
      Serial.println("🔊 MANUAL TEST");
      emitCodedPulse();
    } else if (command == "STATUS" || command == "s") {
      Serial.println("=== ARDUINO STATUS ===");
      Serial.println("🔊 Ultrasound Pin: " + String(ULTRASONIC_PIN));
      Serial.println("🤖 ESP32 Trigger: " + String(ESP_TRIGGER_PIN));
      Serial.println("📍 Pulse Pattern: " + String(OBJECT_ID_PULSES));
      Serial.println("📡 ESP Control: " + String(espCommandActive ? "ACTIVE" : "INACTIVE"));
      Serial.println("🕒 Uptime: " + String(millis()/1000) + "s");
    } else if (command == "HELP" || command == "h") {
      Serial.println("=== ARDUINO COMMANDS ===");
      Serial.println("TEST/t - Manual test pulse");
      Serial.println("STATUS/s - System status");
      Serial.println("HELP/h - This help");
    }
  }

  delay(50);
}
