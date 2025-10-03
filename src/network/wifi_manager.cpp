#include "wifi_manager.h"

void WiFiManager::setupAP() {
    WiFi.softAP("PositioningSystem", "password123");
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    Serial.print("AP SSID: ");
    Serial.println("PositioningSystem");
    Serial.print("Connected stations: ");
    Serial.println(WiFi.softAPgetStationNum());
}

void WiFiManager::setupSTA() {
    Serial.println("🔄 Connecting to PositioningSystem...");

    WiFi.begin("PositioningSystem", "password123");

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ Connected to PositioningSystem!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n❌ FAILED to connect to PositioningSystem!");
        Serial.println("Available networks:");
        // Сканируем доступные сети для диагностики
        int n = WiFi.scanNetworks();
        for (int i = 0; i < n; i++) {
            Serial.println(WiFi.SSID(i) + " - " + String(WiFi.RSSI(i)) + "dB");
        }
    }
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::startUDP() {
    udp.begin(1234);
    Serial.println("UDP started on port 1234");
}

void WiFiManager::sendUDPBroadcast(const String& message) {
    udp.beginPacket("255.255.255.255", 1234);
    udp.print(message);
    udp.endPacket();
}
