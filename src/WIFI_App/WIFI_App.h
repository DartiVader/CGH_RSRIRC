#ifndef WIFI_APP_H
#define WIFI_APP_H

#include <WiFi.h>
#include "config/config.h"

class WIFI_App {
public:
    void begin();
    void printNetworkInfo();
    bool isConnected();
};

#endif
