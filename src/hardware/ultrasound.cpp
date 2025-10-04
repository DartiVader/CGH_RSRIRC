#include "ultrasound.h"
#include "../config/config.h"

void Ultrasound::setupTransmitter() {
    pinMode(ULTRASOUND_TX_PIN, OUTPUT);
    digitalWrite(ULTRASOUND_TX_PIN, LOW); // Гарантируем выключение

    // Настройка ШИМ
    ledcSetup(pwmChannel, pwmFrequency, pwmResolution);
    ledcAttachPin(ULTRASOUND_TX_PIN, pwmChannel);
    ledcWrite(pwmChannel, 0); // Выключаем сигнал

    Serial.println("✅ Transmitter ready on pin " + String(ULTRASOUND_TX_PIN));
    Serial.println("   PWM: " + String(pwmFrequency) + "Hz, channel " + String(pwmChannel));
}

void Ultrasound::setupReceiver() {
    pinMode(ULTRASOUND_RX_PIN, INPUT);
    receiverEnabled = true;

    #ifdef RECEIVER_NODE
    sensorThreshold = 350;
    Serial.println("✅ KY-037 Receiver ready on pin " + String(ULTRASOUND_RX_PIN));
    #elif BEACON_NODE
    #if BEACON_ID == 1
    sensorThreshold = 400;
    Serial.println("✅ KY-037 Receiver ready on pin " + String(ULTRASOUND_RX_PIN));
    #elif BEACON_ID == 2
    sensorThreshold = 300;
    Serial.println("✅ KY-038 Receiver ready on pin " + String(ULTRASOUND_RX_PIN));
    #endif
    #endif
}

void Ultrasound::setupPWM() {
    // Уже реализовано в setupTransmitter()
    Serial.println("📡 PWM setup completed");
}

void Ultrasound::generateTone(bool state) {
    if (state) {
        ledcWrite(pwmChannel, pwmDutyCycle);
    } else {
        ledcWrite(pwmChannel, 0);
    }
}

void Ultrasound::sendBeaconCode(int beaconId) {
    // Упрощенные коды для тестирования
    const int beaconACode[] = {2}; // Просто 2 импульса
    const int beaconBCode[] = {3}; // Просто 3 импульса

    const int* beaconCode;
    int codeLength;

    if (beaconId == 1) {
        beaconCode = beaconACode;
        codeLength = 1;
        Serial.println("   Beacon A: 2 pulses");
    } else {
        beaconCode = beaconBCode;
        codeLength = 1;
        Serial.println("   Beacon B: 3 pulses");
    }

    const int pulseDuration = 100; // Увеличим длительность для теста

    for (int i = 0; i < codeLength; i++) {
        for (int j = 0; j < beaconCode[i]; j++) {
            Serial.println("   Pulse " + String(j+1));

            generateTone(true);
            delay(pulseDuration);
            generateTone(false);

            if (j < beaconCode[i] - 1) {
                delay(100);
            }
        }
    }
}

void Ultrasound::emitPulse(int beaconId) {
    Serial.println("🔊 Legacy ultrasound transmission...");

    #ifdef BEACON_NODE
    for(int i = 0; i < 80; i++) {
        digitalWrite(ULTRASOUND_TX_PIN, HIGH);
        delayMicroseconds(12);
        digitalWrite(ULTRASOUND_TX_PIN, LOW);
        delayMicroseconds(12);
    }
    Serial.println("✅ Legacy pulse completed");
    #endif
}

void Ultrasound::emitCodedPulse(int beaconId) {
    Serial.println("🎯 STARTING CODED PULSE TRANSMISSION");

    // Сначала простой тест - длинный сигнал
    Serial.println("🔊 PHASE 1: Simple 500ms tone");
    generateTone(true);
    delay(500);
    generateTone(false);
    Serial.println("✅ Simple tone completed");

    delay(100);

    // Затем кодированный сигнал
    Serial.println("🔊 PHASE 2: Coded beacon ID");
    sendBeaconCode(beaconId);

    Serial.println("✅ Coded pulse transmission completed");
}

bool Ultrasound::detectPulse() {
    if (!receiverEnabled) return false;

    int sensorValue = getSensorValue();
    bool detected = sensorValue > 100;

    if (detected) {
        Serial.println("🎯 PULSE DETECTED! Value: " + String(sensorValue));
    }

    return detected;
}

int Ultrasound::getSensorValue() {
    if (!receiverEnabled) return 0;

    pinMode(ULTRASOUND_RX_PIN, INPUT);
    delayMicroseconds(10);

    int sensorValue = analogRead(ULTRASOUND_RX_PIN);

    // Диагностика каждые 2 секунды
    static unsigned long lastReport = 0;
    if (millis() - lastReport > 2000) {
        lastReport = millis();

        int readings[5];
        for (int i = 0; i < 5; i++) {
            readings[i] = analogRead(ULTRASOUND_RX_PIN);
            delayMicroseconds(100);
        }

        Serial.println("🔍 SENSOR DIAGNOSTIC - Value: " + String(sensorValue));
    }

    return sensorValue;
}

unsigned long Ultrasound::getLastPulseTime() {
    return pulseStartTime;
}
