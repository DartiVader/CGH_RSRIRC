#ifndef ULTRASOUND_H
#define ULTRASOUND_H

#include <Arduino.h>

class Ultrasound {
public:
    void setupTransmitter();
    void setupReceiver();
    void emitPulse(int beaconId);
    bool detectPulse();
    unsigned long getLastPulseTime();

private:
    unsigned long pulseStartTime = 0;
    bool receiverEnabled = false;
};

#endif
