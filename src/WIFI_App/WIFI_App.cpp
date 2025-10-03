#include "WIFI_App.h"
#include "WiFi.h"

WIFI_App wifiApp;

void WIFI_App::begin() {
    Serial.println("+ Starting WiFi Access Point...");
    
    // Настройка точки доступа
    WiFi.softAP(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL, 0, MAX_CLIENTS);
    
    //Без delay() - можем получить IP до готовности сети из-за асинхронности ESPAsyncWebServer
    delay(1000);
    
    printNetworkInfo();
}

void WIFI_App::printNetworkInfo() {
    Serial.println("+ WiFi AP Started!");
    Serial.print("+ SSID: ");
    Serial.println(WIFI_SSID);
    Serial.print("+ IP Address: ");
    Serial.println(WiFi.softAPIP());
    Serial.print("+ MAC Address: ");
    Serial.println(WiFi.softAPmacAddress());
}

bool WIFI_App::isConnected() {
    return WiFi.softAPgetStationNum() > 0;
}