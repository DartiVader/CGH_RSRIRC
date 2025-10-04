#include <Arduino.h>

// Конфигурация объекта
#define ULTRASONIC_PIN 8
#define STATUS_LED_PIN 13
#define PULSE_INTERVAL 2000  // 2 секунды между импульсами

// Параметры ШИМ
const int pwmChannel = 0;
const int pwmFrequency = 40000; // 40 kHz
const int pwmResolution = 8;
const int pwmDutyCycle = 127; // 50% заполнение

// Код объекта (простой одиночный импульс)
const int objectCode[] = {1};
const int codeLength = 1;

void setupPWM() {
  // Настройка ШИМ для генерации 40 кГц
  ledcSetup(pwmChannel, pwmFrequency, pwmResolution);
  ledcAttachPin(ULTRASONIC_PIN, pwmChannel);
  ledcWrite(pwmChannel, 0); // Выключаем сигнал

  Serial.println("✅ PWM initialized: 40kHz on pin " + String(ULTRASONIC_PIN));
}

void generateTone(bool state) {
  if (state) {
    ledcWrite(pwmChannel, pwmDutyCycle);
  } else {
    ledcWrite(pwmChannel, 0);
  }
}

void emitCodedPulse() {
  Serial.println("🚀 EMITTING CODED ULTRASOUND PULSE");

  const int pulseDuration = 15;    // Длительность импульса (мс)
  const int betweenPulseDelay = 5; // Пауза между импульсами

  for (int i = 0; i < codeLength; i++) {
    for (int j = 0; j < objectCode[i]; j++) {
      generateTone(true);
      delay(pulseDuration);
      generateTone(false);
      if (j < objectCode[i] - 1) {
        delay(betweenPulseDelay);
      }
    }
  }

  Serial.println("✅ Coded pulse completed");
}

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  Serial.begin(115200);

  Serial.println("=== 🎯 ULTRASOUND OBJECT - CODED PULSES ===");
  Serial.println("🔊 40kHz PWM with coded identification");

  setupPWM();
}

void loop() {
  // Мигание светодиодом в режиме ожидания
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 500) {
    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
    lastBlink = millis();
  }

  // Периодическая отправка кодированных импульсов
  static unsigned long lastPulse = 0;
  if (millis() - lastPulse > PULSE_INTERVAL) {
    lastPulse = millis();

    emitCodedPulse();

    // Быстрое мигание для подтверждения передачи
    for(int i = 0; i < 3; i++) {
      digitalWrite(STATUS_LED_PIN, HIGH);
      delay(80);
      digitalWrite(STATUS_LED_PIN, LOW);
      delay(80);
    }
  }

  delay(100);
}
