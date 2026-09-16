#include <LittleFS.h>
#include "TaskRoutes.h"

void registerTaskRoutes(AsyncWebServer& server, TaskManager& taskManager) {
    server.on("/api/tasks", HTTP_GET, [&taskManager](AsyncWebServerRequest *request) {
        request->send(200, "application/json", taskManager.toJson());
    });

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/index.html", "text/html");
    });

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "File tidak ada\n");
    });

    server.serveStatic("/assets/", LittleFS, "/assets/")
          .setCacheControl("no-store");
}