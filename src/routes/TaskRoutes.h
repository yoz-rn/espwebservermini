#pragma once

#include "TaskManager.h"
#include "ResponseHelper.h"

void registerTaskRoutes(AsyncWebServer& server, TaskManager& taskManager);
