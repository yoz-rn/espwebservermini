#pragma once
#include <ESPAsyncWebServer.h>

#include "FileManager.h"

void registerFileRoutes(AsyncWebServer& server, FileManager& fileManager);