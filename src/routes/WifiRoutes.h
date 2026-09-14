#pragma once
#include <ESPAsyncWebServer.h>

#include "NetworkManager.h"

void registerWifiRoutes(AsyncWebServer& server, NetworkManager& networkManager);