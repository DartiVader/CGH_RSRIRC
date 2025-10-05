#include "positioning.h"
#include "../config/config.h"
#include <math.h>

void PositioningSystem::begin() {
    Serial.println("Positioning System initialized");
    currentPosition.x = 0;
    currentPosition.y = 0;
    currentPosition.accuracy = 100.0;
    currentPosition.timestamp = millis();
    currentPosition.valid = false;
}

void PositioningSystem::update() {
    // Можно добавить фильтрацию позиций
}

Position PositioningSystem::getCurrentPosition() {
    return currentPosition;
}

Position PositioningSystem::trilaterate(float distances[4]) { // 4 расстояния: 3 маяка + объект
    Position result;
    result.timestamp = millis();
    result.valid = false; // по умолчанию невалидно
 // Используем только 2 маяка
    float x1 = BEACON1_X, y1 = BEACON1_Y;
    float x2 = BEACON2_X, y2 = BEACON2_Y;

    float d1 = distances[0]; // до маяка 1
    float d2 = distances[1]; // до маяка 2
    // distances[2] - расстояние до объекта (не используется напрямую в 2-точечной трилатерации)

    // Упрощенная трилатерация для 2 точек
    // В реальной системе нужен более сложный алгоритм
    // Пока используем упрощенный подход

    // Вычисляем позицию как пересечение двух окружностей
    float dx = x2 - x1;
    float dy = y2 - y1;
    float d = sqrt(dx*dx + dy*dy);

    // Проверка возможности решения
    if (d > (d1 + d2) || d < fabs(d1 - d2)) {
        Serial.println("Trilateration error: No intersection");
        return result;
    }

    // Вычисления
    float a = (d1*d1 - d2*d2 + d*d) / (2.0*d);
    float h = sqrt(d1*d1 - a*a);

    float x = x1 + a * (dx / d);
    float y = y1 + a * (dy / d);

    // Два возможных решения
    float x3 = x + h * (dy / d);
    float y3 = y - h * (dx / d);

    // Выбираем решение с положительной Y координатой
    result.x = x3;
    result.y = y3;

    // Вычисляем точность
    float calc_d1 = sqrt(pow(result.x - x1, 2) + pow(result.y - y1, 2));
    float calc_d2 = sqrt(pow(result.x - x2, 2) + pow(result.y - y2, 2));
    result.accuracy = (fabs(calc_d1 - d1) + fabs(calc_d2 - d2)) / 2.0;
    result.valid = (result.accuracy < 50.0);

    Serial.println("2-Beacon Trilateration - X: " + String(result.x) + " Y: " + String(result.y) +
                  " Accuracy: " + String(result.accuracy) + " Valid: " + String(result.valid));

    currentPosition = result;
    return result;
}

void PositioningSystem::calibrateSystem() {
    Serial.println("Calibrating positioning system...");
    currentPosition.valid = false;
}
