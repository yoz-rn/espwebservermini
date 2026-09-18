#include "GameRoutes.h"

namespace {
    bool isValidGame(const String& game) {
        return game == "snek" || game == "tetris";
    }
}

void registerGameRoutes(AsyncWebServer& server, GameManager& gameManager) {
    
    // Server API to GET High Score
    server.on("/api/game/score", HTTP_GET, GAME_SERVER {
        if (!request->hasParam("game")) {
            request->send(400, APP_JSON, buildStatusJson(false, "parameter path required"));
            return;
        }

        String game = request->getParam("game")->value();

        if (isValidGame(game)) {
            request->send(400, APP_JSON, buildStatusJson(false, "unknown game"));
            return;
        }
        
        JsonDocument doc;
        doc["success"] = true;
        doc["highscore"] = gameManager.getHighScore(game);

        String output;
        serializeJson(doc, output);
        request->send(200, APP_JSON, output);
    });

    // Server API to POST Score
    server.on("api/game/score", HTTP_POST, GAME_SERVER {
        if (!request->hasParam("game") || !request->hasParam("score")) {
            request->send(400, APP_JSON, buildStatusJson(false, "parameter game and score required"));
            return;
        }

        String game = request->getParam("game")->value();

        if (!isValidGame(game)) {
            request->send(400, APP_JSON, buildStatusJson(false, "unknown game"));
            return;        
        }

        uint32_t score = request->getParam("score")->value().toInt();
        uint32_t highScore;

        if (!gameManager.SubmitScore(game, score, highScore)) {
            request->send(500, APP_JSON, buildStatusJson(false,"failed to save score"));
            return;
        }

        JsonDocument doc;
        doc["success"] = true;
        doc["highScore"] = highScore;
        doc["isNewRecord"] = (score == highScore);

        String output;
        serializeJson(doc, output);
        request->send(200, APP_JSON, output);   
    });
}
