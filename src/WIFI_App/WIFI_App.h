// Создание точки доступа WIFI

#ifndef WIFI_APP_H
#define WIFI_APP_H

#include "../config.h"

class WIFI_App {
public:
    void begin();
    void printNetworkInfo();
    bool isConnected();
};

extern WIFI_App wifiApp;

#endif