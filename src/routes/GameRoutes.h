
#include <Arduino.h>
#include "ResponseHelper.h"
#include "GameManager.h"

#define GAME_SERVER [&gameManager](AsyncWebServerRequest *request)

void registerGameRoutes(AsyncWebServer& server, GameManager& game);