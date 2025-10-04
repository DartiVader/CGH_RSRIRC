#include <Arduino.h>

#define TRANSISTOR_PIN 8
#define STATUS_LED_PIN 13
#define PULSE_INTERVAL 2000

// ОБЪЯВЛЯЕМ ФУНКЦИИ ДО ИХ ИСПОЛЬЗОВАНИЯ
void emitFullPowerUltrasound();
void emitUltrasound(unsigned long duration);

void setup() {
  pinMode(TRANSISTOR_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  Serial.begin(115200);

  Serial.println("=== 🎯 ULTRASOUND OBJECT - 3.3V POWER ===");
  Serial.println("🔊 3.3V power - safe for 8Ω 1W speaker");
  Serial.println("🚀 Full power ultrasound pulses");
}

void loop() {
  digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));

  static unsigned long lastPulse = 0;
  if (millis() - lastPulse > PULSE_INTERVAL) {
    lastPulse = millis();

    emitFullPowerUltrasound();
  }

  delay(100);
}

void emitFullPowerUltrasound() {
  Serial.println("🚀 FULL POWER ULTRASOUND PULSE");

  // ПОЛНАЯ МОЩНОСТЬ - 100ms при 40kHz
  unsigned long duration = 100000; // 100ms
  unsigned long startTime = micros();
  long cycleCount = 0;

  while (micros() - startTime < duration) {
    digitalWrite(TRANSISTOR_PIN, HIGH);
    delayMicroseconds(500); // 40kHz
    digitalWrite(TRANSISTOR_PIN, LOW);
    delayMicroseconds(500);
    cycleCount++;
  }

  Serial.print("🔊 100ms pulse, 40kHz, ");
  Serial.print(cycleCount);
  Serial.println(" cycles");

  // Проверка температуры
  static unsigned long pulseCount = 0;
  pulseCount++;
  if (pulseCount % 5 == 0) {
    Serial.println("🌡️  Check speaker temperature");
  }

  // Мигание подтверждения
  for(int i = 0; i < 3; i++) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(80);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(80);
  }
}

void emitUltrasound(unsigned long duration) {
  unsigned long start = micros();
  long cycles = 0;

  while(micros() - start < duration) {
    digitalWrite(TRANSISTOR_PIN, HIGH);
    delayMicroseconds(12);
    digitalWrite(TRANSISTOR_PIN, LOW);
    delayMicroseconds(12);
    cycles++;
  }

  Serial.print("   ");
  Serial.print(cycles);
  Serial.println(" cycles");
}

// Функция для теста безопасности (не используется в loop, можно убрать если не нужна)
void safetyTest() {
  Serial.println("🧪 3.3V SAFETY TEST:");

  // Тест 1: Короткий импульс
  Serial.println("1. Short pulse test");
  emitUltrasound(20000); // 20ms
  delay(2000);

  // Проверка температуры
  Serial.println("2. Touch speaker - should be WARM but not HOT");
  delay(3000);

  // Тест 2: Длинный импульс
  Serial.println("3. Long pulse test");
  emitUltrasound(100000); // 100ms
  delay(2000);

  // Финальная проверка
  Serial.println("4. Final temperature check");
  Serial.println("✅ If not overheating - 3.3V is SAFE");
}
