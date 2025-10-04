#include "web_server.h"
#include <LittleFS.h>
#include <Arduino.h>

WebService webService;

void WebService::begin() {
    Serial.println("🌐 Starting Async Web Server...");
    
    LittleFS.begin();
    
    webSocket.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
        this->onWebSocketEvent(server, client, type, arg, data, len);
    });
    server.addHandler(&webSocket);
    
    setupRoutes();
    
    server.begin();
    Serial.println("✅ Async Web Server started on port " + String(WEB_PORT));
}

void WebService::setupRoutes() {
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    
    server.onNotFound([](AsyncWebServerRequest* request) {
        if (request->method() == HTTP_GET) {
            request->send(LittleFS, "/index.html", "text/html");
        } else {
            request->send(404);
        }
    });
}

void WebService::onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
    switch(type) {
        case WS_EVT_CONNECT:
            Serial.println("🔗 Client connected");
            // Отправляем текущую позицию при подключении
            client->text(getJSONPosition());
            break;
            
        case WS_EVT_DISCONNECT:
            Serial.println("🔌 Client disconnected");
            break;
            
        case WS_EVT_DATA:
            // Обрабатываем команды от клиента
            if (len > 0) {
                String message = String((char*)data).substring(0, len);
                Serial.printf("📨 Received command: %s\n", message.c_str());
                
                if (message == "START") {
                    client->text("{\"type\":\"status\",\"message\":\"Measurements started\"}");
                    Serial.println("🚀 Measurements STARTED");
                } else if (message == "STOP") {
                    client->text("{\"type\":\"status\",\"message\":\"Measurements stopped\"}");
                    Serial.println("🛑 Measurements STOPPED");
                }
            }
            break;
            
        case WS_EVT_ERROR:
            Serial.printf("❌ WebSocket error: %s\n", (char*)data);
            break;
    }
}

String WebService::getJSONPosition() {
    StaticJsonDocument<200> doc;
    doc["x"] = currentPosition.x;
    doc["y"] = currentPosition.y;
    doc["accuracy"] = currentPosition.accuracy;
    doc["valid"] = currentPosition.valid;
    doc["timestamp"] = currentPosition.timestamp;
    
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

void WebService::updatePosition(const PositionData& newPosition) {
    currentPosition = newPosition;
    broadcastPosition();
}

void WebService::broadcastPosition() {
    String json = getJSONPosition();
    webSocket.textAll(json);
}