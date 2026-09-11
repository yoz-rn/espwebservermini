#pragma once
#include <ESPAsyncWebServer.h>

#include "NetworkManager.h"

struct WifiTestParams {
    AsyncWebServerRequest* request;
    NetworkManager* networkManager;
    String ssid;
    String password;
};

void registerWifiRoutes(AsyncWebServer& server, NetworkManager& NetworkManager);