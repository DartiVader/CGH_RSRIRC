#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <WiFiUdp.h>

class WiFiManager {
public:
    void setupAP();
    void setupSTA();
    bool isConnected();
    void startUDP();
    void sendUDPBroadcast(const String& message);

    WiFiUDP udp;

//private:
  //  WiFiUDP udp;
};

#endif
