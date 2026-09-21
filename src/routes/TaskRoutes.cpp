#include <LittleFS.h>
#include "TaskRoutes.h"

void registerTaskRoutes(AsyncWebServer& server, TaskManager& taskManager) {
    server.on("/api/tasks", HTTP_GET, [&taskManager](AsyncWebServerRequest *request) {
        String body;
        if (!taskManager.toJson(body)) {
            request->send(500, APP_JSON, buildStatusJson(false, "Failed to read tasks list"));
            return;
        }
        request->send(200, APP_JSON, body);
    });
}