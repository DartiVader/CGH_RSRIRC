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
    Serial.println("📤 UDP Broadcast: " + message);

    // Пробуем разные варианты broadcast адресов
    IPAddress broadcastIP = IPAddress(255, 255, 255, 255);
    IPAddress subnetBroadcast = IPAddress(192, 168, 4, 255);

    // Получаем локальный IP для диагностики
    IPAddress localIP = WiFi.localIP();

    Serial.println("   Local IP: " + localIP.toString());
    Serial.println("   Trying broadcast: 255.255.255.255");

    // Основной broadcast
    udp.beginPacket(broadcastIP, 1234);
    udp.print(message);
    bool result1 = udp.endPacket();
    Serial.println("   Result 255.255.255.255: " + String(result1 ? "SUCCESS" : "FAILED"));

    delay(10);

    // Broadcast для подсети AP
    Serial.println("   Trying broadcast: 192.168.4.255");
    udp.beginPacket(subnetBroadcast, 1234);
    udp.print(message);
    bool result2 = udp.endPacket();
    Serial.println("   Result 192.168.4.255: " + String(result2 ? "SUCCESS" : "FAILED"));

    // Если мы в STA режиме, пробуем broadcast для нашей подсети
    if (WiFi.getMode() == WIFI_STA) {
        IPAddress gateway = WiFi.gatewayIP();
        IPAddress subnet = WiFi.subnetMask();

        // Вычисляем broadcast адрес для текущей подсети
        IPAddress networkBroadcast = IPAddress(
            gateway[0] | ~subnet[0],
            gateway[1] | ~subnet[1],
            gateway[2] | ~subnet[2],
            gateway[3] | ~subnet[3]
        );

        Serial.println("   Trying network broadcast: " + networkBroadcast.toString());
        udp.beginPacket(networkBroadcast, 1234);
        udp.print(message);
        bool result3 = udp.endPacket();
        Serial.println("   Result " + networkBroadcast.toString() + ": " + String(result3 ? "SUCCESS" : "FAILED"));
    }

    // Отправка напрямую на известные IP (для тестирования)
    Serial.println("   Direct send to common IPs...");
    IPAddress commonIPs[] = {
        IPAddress(192, 168, 4, 1),  // Ресивер AP
        IPAddress(192, 168, 4, 2),
        IPAddress(192, 168, 4, 11), // Маяк 1
        IPAddress(192, 168, 4, 12)  // Маяк 2
    };

    for (int i = 0; i < 4; i++) {
        udp.beginPacket(commonIPs[i], 1234);
        udp.print(message);
        bool result = udp.endPacket();
        if (result) {
            Serial.println("   Direct to " + commonIPs[i].toString() + ": SUCCESS");
        }
    }
}
