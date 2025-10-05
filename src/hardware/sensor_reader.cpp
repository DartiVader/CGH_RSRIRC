#include "sensor_reader.h"

void SensorReader::begin(int sensorPin, const String& sensorType) {
    pin = sensorPin;
    type = sensorType;
    pinMode(pin, INPUT);

    Serial.println("Sensor " + sensorType + " initialized on pin " + String(pin));
    calibrate();
}

void SensorReader::calibrate() {
    Serial.println("Calibrating " + type + " sensor...");

    long sum = 0;
    const int samples = 100;

    for (int i = 0; i < samples; i++) {
        sum += readSensor();
        delay(10);
    }

    baseline = sum / samples;
    calibrated = true;

    Serial.println(type + " baseline: " + String(baseline));
}

int SensorReader::readSensor() {
    return analogRead(pin);
}

bool SensorReader::detectPulse(unsigned long timeout) {
    if (!calibrated) calibrate();

    unsigned long startTime = micros();
    const int threshold = isKY037() ? baseline + 100 : baseline + 80; // KY-037 более чувствителен
    const int confirmationCount = isKY037() ? 2 : 3; // KY-037 требует меньше подтверждений

    int positiveReadings = 0;

    while (micros() - startTime < timeout) {
        int sensorValue = readSensor();

        if (sensorValue > threshold) {
            positiveReadings++;

            if (positiveReadings >= confirmationCount) {
                pulseDetectionTime = micros();
                lastAmplitude = sensorValue - baseline;

                Serial.println(type + " PULSE DETECTED! Amplitude: " + String(lastAmplitude) +
                              ", Time: " + String(pulseDetectionTime) + "μs");
                return true;
            }
        } else {
            positiveReadings = 0;
        }

        delayMicroseconds(50); // Небольшая задержка между чтениями
    }

    return false;
}

unsigned long SensorReader::getLastPulseTime() {
    return pulseDetectionTime;
}

float SensorReader::getLastAmplitude() {
    return lastAmplitude;
}
