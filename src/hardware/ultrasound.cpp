#include "ultrasound.h"
#include "config/config.h"

void Ultrasound::setupTransmitter() {
    pinMode(ULTRASOUND_TX_PIN, OUTPUT);
    Serial.println("Ultrasound transmitter ready");
}

void Ultrasound::setupReceiver() {
    pinMode(ULTRASOUND_RX_PIN, INPUT);
    receiverEnabled = true;
    Serial.println("Ultrasound receiver ready");
}

void Ultrasound::emitPulse(int beaconId) {
    // Генерация ультразвукового импульса 40 кГц
    for(int i = 0; i < 20; i++) { // 20 импульсов = 500 мкс
        digitalWrite(ULTRASOUND_TX_PIN, HIGH);
        delayMicroseconds(12);  // Полупериод 40 кГц
        digitalWrite(ULTRASOUND_TX_PIN, LOW);
        delayMicroseconds(12);
    }
    Serial.printf("Beacon %d: Ultrasound pulse emitted\n", beaconId);
}

bool Ultrasound::detectPulse() {
    if (!receiverEnabled) return false;

    // Простая детекция по аналоговому значению
    int sensorValue = analogRead(ULTRASOUND_RX_PIN);
    return sensorValue > 500; // Пороговое значение
}

unsigned long Ultrasound::getLastPulseTime() {
    return pulseStartTime;
}
