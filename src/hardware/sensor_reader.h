#ifndef SENSOR_READER_H
#define SENSOR_READER_H

#include <Arduino.h>

class SensorReader {
public:
    void begin(int sensorPin, const String& sensorType);
    bool detectPulse(unsigned long timeout = 100000); // timeout в микросекундах
    unsigned long getLastPulseTime();
    float getLastAmplitude();
    void calibrate();

private:
    int pin;
    String type;
    unsigned long pulseDetectionTime = 0;
    float lastAmplitude = 0;
    int baseline = 0;
    bool calibrated = false;

    int readSensor();
    bool isKY037() { return type == "KY-037"; }
};

#endif
