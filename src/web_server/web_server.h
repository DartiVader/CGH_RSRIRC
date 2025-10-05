#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "../config/config.h"

class WebService {
private:
    AsyncWebServer server;
    AsyncWebSocket webSocket;
    PositionData currentPosition;
    bool measurementsActive = false;
    int activeBeaconId = -1;
    int detectedSensors = 0;

    void setupRoutes();
    void onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);
    void broadcastSystemData();
    String getJSONSystemData();
    String getHTMLPage();

    // ПРОСТАЯ РЕАЛИЗАЦИЯ ДАТЧИКОВ
    int readRealSensors();

public:
    WebService() : server(80), webSocket("/ws") {}

    void begin();
    void updatePosition(const PositionData& newPosition);
    void setActiveBeacon(int beaconId) { activeBeaconId = beaconId; }
    void setDetectedSensors(int count) { detectedSensors = count; }
    bool isMeasuring() { return measurementsActive; }
    void updateMeasurementStatus(bool measuring) {
        measurementsActive = measuring;
        Serial.println("WebService measurements: " + String(measuring ? "ACTIVE" : "INACTIVE"));
    }
};

#endif
