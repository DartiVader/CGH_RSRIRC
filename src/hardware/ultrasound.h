#ifndef ULTRASOUND_H
#define ULTRASOUND_H

#include <Arduino.h>

class Ultrasound {
public:
    void setupTransmitter();
    void setupReceiver();
    void emitPulse(int beaconId);
    void emitCodedPulse(int beaconId);
    bool detectPulse();
    unsigned long getLastPulseTime();
    int getSensorValue();

    void setupPWM();
    void generateTone(bool state);
    void sendBeaconCode(int beaconId);

    // Новые методы для разных типов зуммеров
    void emitPulseKY006(int beaconId);
    void emitPulseKY012(int beaconId);
    void emitSimplePulses(int beaconId);
    void emitPWMPulses(int beaconId);

private:
    unsigned long pulseStartTime = 0;
    bool receiverEnabled = false;
    int sensorThreshold = 300;

    const int pwmChannel = 0;
    const int pwmFrequency = 40000;
    const int pwmResolution = 8;
    const int pwmDutyCycle = 127;
};

#endif
