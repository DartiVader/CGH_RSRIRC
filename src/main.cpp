#include <Arduino.h>   // обязательно для функций delay(), pinMode и т.д.

// Конфигурация маяка
#define ULTRASONIC_PIN 18
#define LED_STATUS_PIN 2 // Светодиод для индикации работы

// Параметры сигнала
const int freq = 1000; // 40 kHz
const int channel = 0; // PWM-канал (0-15)
const int resolution = 8; // Разрешение бит (1-16). 8 бит = 256 градаций.

// Уникальный код маяка (последовательность импульсов)
// Например, для маяка "A": 3 импульса, пауза, 1 импульс.
// пауза между любой пачкой импульсов
const int beacon_id[] = {3, 1}; // Массив: количество импульсов в пачке
const int id_length = 2; // Длина массива идентификатора
const int pulse_duration = 10; // Длительность одного импульса в миллисекундах
const int between_pulse_delay = 10; // Пауза между импульсами в пачке (мс)
const int between_id_delay = 100; // Пауза между повторами ID (мс)

void setup() {
  pinMode(LED_STATUS_PIN, OUTPUT);
  
  // Настройка ШИМ для генерации 40 кГц
  ledcSetup(channel, freq, resolution);
  ledcAttachPin(ULTRASONIC_PIN, channel);
  
  // Гарантируем, что сигнал изначально выключен
  ledcWrite(channel, 0);
}

void loop() {
  // Цикл передачи уникального кода
  // id_length = beacon_id.length
  for (int i = 0; i < id_length; i++) {
    // Передаем пачку импульсов
    for (int j = 0; j < beacon_id[i]; j++) {
      ledcWrite(channel, 127); // Включаем ШИМ (50% заполнение)
      delay(pulse_duration);
      ledcWrite(channel, 0);   // Выключаем ШИМ
      delay(between_pulse_delay);
    }
    // Ждем перед следующей пачкой в коде
    delay(between_id_delay);
  }
  
  // Длинная пауза перед следующим циклом передачи всего ID
  digitalWrite(LED_STATUS_PIN, HIGH); // Индикация передачи
  delay(500); // Пауза между посылками ID (можно регулировать)
  digitalWrite(LED_STATUS_PIN, LOW);
  delay(500);
}