#ifndef POSITIONING_H
#define POSITIONING_H

#include <Arduino.h>

struct Position {
    float x;        // координата X в см
    float y;        // координата Y в см
    float accuracy; // точность в см
    uint32_t timestamp;
    bool valid;     // ДОБАВЛЯЕМ ЭТО ПОЛЕ
};

class PositioningSystem {
public:
    void begin();
    void update();
    Position getCurrentPosition();
    Position trilaterate(float distances[4]); // меняем на 4 расстояния
    void calibrateSystem();

private:
    Position currentPosition;
    // Убираем старые позиции, используем определения из config.h
    float speedOfSound = 34300.0; // см/с
};

#endif
