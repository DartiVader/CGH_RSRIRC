#ifndef POSITIONING_H
#define POSITIONING_H

#include <Arduino.h>

struct Position {
    float x;        // координата X в см
    float y;        // координата Y в см
    float accuracy; // точность в см
    uint32_t timestamp;
};

class PositioningSystem {
public:
    void begin();
    void update();
    Position getCurrentPosition();

    // Алгоритмы которые будем реализовывать
    Position trilaterate(float distances[3]);
    void calibrateSystem();

private:
    Position currentPosition;
    float beaconPositions[3][2] = {{0, 0}, {400, 0}, {200, 300}};
    float speedOfSound = 34300.0; // см/с
};

#endif
