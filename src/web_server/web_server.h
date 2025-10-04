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

    void setupRoutes();
    void onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);
    void broadcastPosition();
    String getJSONPosition();
    String getHTMLPage();

public:
    WebService() : server(80), webSocket("/ws") {}

    void begin();
    void updatePosition(const PositionData& newPosition);
    bool isMeasuring() { return measurementsActive; }
};

#endif
