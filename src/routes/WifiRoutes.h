#pragma once

#include "NetworkManager.h"
#include "ResponseHelper.h"

#define WIFI_SERVER [&networkManager](AsyncWebServerRequest *request)

void registerWifiRoutes(AsyncWebServer& server, NetworkManager& networkManager);