#pragma once
#include <ESPAsyncWebServer.h>

#include "FileManager.h"
#include "ResponseHelper.h"

void registerFileRoutes(AsyncWebServer& server, FileManager& fileManager);