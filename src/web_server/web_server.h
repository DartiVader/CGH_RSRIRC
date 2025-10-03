#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "../config.h"

class WebService {
private:
    AsyncWebServer server;
    AsyncWebSocket webSocket;
    PositionData currentPosition;

    void setupRoutes();
    void onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);
    void broadcastPosition();
    String getJSONPosition();

public:
    WebService() : server(WEB_PORT), webSocket(WS_PATH) {}
    
    void begin();
    void updatePosition(const PositionData& newPosition);
};

extern WebService webService;

#endif