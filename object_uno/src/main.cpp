#include <Arduino.h>

// Конфигурация для Arduino Uno с усилителем PAM8403
#define ULTRASONIC_PIN 8        // Pin 8 → L усилителя
#define STATUS_LED_PIN 13       // Встроенный светодиод
#define PULSE_INTERVAL 2000     // 2 секунды между импульсами (в миллисекундах)

void testAmplifier();
void emitSinglePulse();
void emitCodedPulse();
void emitCalibrationTone();


// Параметры тона (Arduino Uno не поддерживает ШИМ 40kHz напрямую)
const int TONE_FREQUENCY = 40000; // 40 kHz
const int PULSE_DURATION = 15;    // Длительность импульса в мс

// Код объекта (идентификационные импульсы)
const int OBJECT_ID_PULSES = 3;   // 3 импульса для идентификации

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(ULTRASONIC_PIN, OUTPUT);
  digitalWrite(ULTRASONIC_PIN, LOW); // Гарантируем выключение

  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== 🎯 ULTRASOUND OBJECT - ARDUINO UNO ===");
  Serial.println("🔊 Amplifier: PAM8403");
  Serial.println("📌 Connection: Pin 8 → L, 5V → +, GND → - & G");
  Serial.println("🔈 Speaker: 8Ω 1W");
  Serial.println("📍 Object ID: " + String(OBJECT_ID_PULSES) + " pulses");

  // Тестовый сигнал при запуске
  Serial.println("\n🔊 INITIAL AMPLIFIER TEST...");
  testAmplifier();

  Serial.println("\n✅ Object initialization completed!");
  Serial.println("📡 Ready to emit pulses every " + String(PULSE_INTERVAL/1000) + " seconds");
}

void testAmplifier() {
  Serial.println("🎵 Testing amplifier and speaker...");
  digitalWrite(STATUS_LED_PIN, HIGH);

  // Генерируем тестовый тон 1 секунду
  for(int i = 0; i < 3; i++) {
    Serial.println("   Beep " + String(i+1));
    emitSinglePulse();
    delay(300);
  }

  digitalWrite(STATUS_LED_PIN, LOW);
  Serial.println("✅ Amplifier test completed");
}

void emitSinglePulse() {
  // Генерируем 40kHz сигнал программно
  unsigned long startTime = micros();
  while (micros() - startTime < PULSE_DURATION * 1000) { // Конвертируем в микросекунды
    digitalWrite(ULTRASONIC_PIN, HIGH);
    delayMicroseconds(12); // Полупериод 40kHz ≈ 12.5μs
    digitalWrite(ULTRASONIC_PIN, LOW);
    delayMicroseconds(12);
  }
}

void emitCodedPulse() {
  Serial.println("🚀 EMITTING CODED ULTRASOUND PULSE");
  Serial.println("   Pattern: " + String(OBJECT_ID_PULSES) + " pulses");

  digitalWrite(STATUS_LED_PIN, HIGH); // Индикация передачи

  for (int i = 0; i < OBJECT_ID_PULSES; i++) {
    Serial.println("   🔊 Pulse " + String(i+1) + "/" + String(OBJECT_ID_PULSES));
    emitSinglePulse();

    // Пауза между импульсами
    if (i < OBJECT_ID_PULSES - 1) {
      delay(10); // 10ms между импульсами
    }
  }

  digitalWrite(STATUS_LED_PIN, LOW);
  Serial.println("✅ Coded pulse completed");
}

void emitCalibrationTone() {
  Serial.println("🎵 CALIBRATION TONE - 500ms");
  digitalWrite(STATUS_LED_PIN, HIGH);

  // Длительный тон для калибровки
  unsigned long startTime = millis();
  while (millis() - startTime < 500) {
    digitalWrite(ULTRASONIC_PIN, HIGH);
    delayMicroseconds(12);
    digitalWrite(ULTRASONIC_PIN, LOW);
    delayMicroseconds(12);
  }

  digitalWrite(STATUS_LED_PIN, LOW);
  Serial.println("✅ Calibration completed");
}

void loop() {
  // Мигание светодиодом в режиме ожидания
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 1000) {
    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
    lastBlink = millis();
  }

  // Периодическая отправка кодированных импульсов
  static unsigned long lastPulse = 0;
  if (millis() - lastPulse > PULSE_INTERVAL) {
    lastPulse = millis();

    Serial.println("\n=== 🚀 PULSE CYCLE ===");
    emitCodedPulse();

    // Диагностика
    Serial.println("⏰ Next pulse in: " + String(PULSE_INTERVAL/1000) + "s");
    Serial.println("🕒 Uptime: " + String(millis()/1000) + "s");
  }

  // Обработка Serial команд для тестирования
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command == "TEST" || command == "t") {
      Serial.println("🔊 MANUAL TEST PULSE");
      emitCodedPulse();
    } else if (command == "CALIBRATE" || command == "c") {
      emitCalibrationTone();
    } else if (command == "STATUS" || command == "s") {
      Serial.println("=== OBJECT STATUS ===");
      Serial.println("🔊 Amplifier: PAM8403");
      Serial.println("🔈 Speaker: 8Ω 1W");
      Serial.println("📌 Pin: " + String(ULTRASONIC_PIN));
      Serial.println("📍 Pulse pattern: " + String(OBJECT_ID_PULSES) + " pulses");
      Serial.println("⏰ Interval: " + String(PULSE_INTERVAL/1000) + " seconds");
      Serial.println("🕒 Uptime: " + String(millis()/1000) + "s");
    } else if (command == "SINGLE" || command == "1") {
      Serial.println("🔊 SINGLE PULSE");
      emitSinglePulse();
    } else if (command == "HELP" || command == "h") {
      Serial.println("=== AVAILABLE COMMANDS ===");
      Serial.println("TEST/t - Manual test pulse");
      Serial.println("CALIBRATE/c - Calibration tone");
      Serial.println("STATUS/s - System status");
      Serial.println("SINGLE/1 - Single pulse");
      Serial.println("HELP/h - This help");
    }
  }

  delay(50);
}
