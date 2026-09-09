#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

#include "secrets.h"
#include "NetworkManager.h"
#include "TaskManager.h"

#include "routes/TaskRoutes.h"

NetworkManager myNetwork(ssid, password);
TaskManager myTaskManager;

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);
  if (!myNetwork.beginAP()) {
    Serial.println("[Main] FATAL: Network gagal, sistem tidak bisa lanjut.");
    return;
  }
  Serial.print("[Network] IP Access Point: ");
  Serial.println(myNetwork.getIP());

  if(!LittleFS.begin(true)) {
    Serial.println("[Main] FATAL: LittleFS gagal mount.");
    return;
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  }
  ); 
    

  registerTaskRoutes(server, myTaskManager);
  server.serveStatic("/", LittleFS, "/");



  server.begin();
  Serial.println("[Server] HTTP Server berjalan di latar belakang.");

}

void loop() {
  vTaskDelete(NULL);
}