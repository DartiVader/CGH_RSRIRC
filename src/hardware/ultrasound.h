#ifndef ULTRASOUND_H
#define ULTRASOUND_H

#include <Arduino.h>

class Ultrasound {
public:
    void setupTransmitter();
    void setupReceiver();
    void emitPulse(int beaconId);
    void emitCodedPulse(int beaconId);  // Новая функция для кодированных импульсов
    bool detectPulse();
    unsigned long getLastPulseTime();
    int getSensorValue();

    // Функции для работы с ШИМ
    void setupPWM();
    void generateTone(bool state);
    void sendBeaconCode(int beaconId);

private:
    unsigned long pulseStartTime = 0;
    bool receiverEnabled = false;
    int sensorThreshold = 300;

    // Параметры ШИМ
    const int pwmChannel = 0;
    const int pwmFrequency = 40000; // 40 kHz
    const int pwmResolution = 8;
    const int pwmDutyCycle = 127; // 50% заполнение
};

#endif
