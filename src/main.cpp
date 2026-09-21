// This include is for prebuilt library/registry
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

// This is the sub-system module
#include "secrets.h"
#include "NetworkManager.h"
#include "TaskManager.h"
#include "GameManager.h"

// This is the API/routes
#include "routes/TaskRoutes.h"
#include "routes/WifiRoutes.h"
#include "routes/FileRoutes.h"
#include "routes/GameRoutes.h"

// 
NetworkManager myNetwork(ssid, password);
TaskManager myTask;
FileManager myFile;
GameManager myGame;

AsyncWebServer server(80);


/* Honestly, idk why this indent
 * only use 2 spaces instead of 4
 */

void setup() {
  Serial.begin(115200);

  if (!myNetwork.beginAP()) {
    Serial.println("[Main] FATAL: Network gagal, sistem tidak bisa lanjut.");
    return;
  }
  Serial.print("[Network] IP Access Point: ");
  Serial.println(myNetwork.getIP());

  if (myNetwork.beginSTA()) {
    Serial.print("[Network] IP STA (router): ");
    Serial.println(myNetwork.getSTAIP());
  } 
  else Serial.println("[Network] STA gagal connect, lanjut pakai AP.");

  if(!LittleFS.begin(true)) {
    Serial.println("[Main] FATAL: LittleFS gagal mount.");
    return;
  }
  else {
    Serial.printf("[LittleFS] Total: %u bytes | Used: %u bytes | Free: %u bytes\n",
              LittleFS.totalBytes(),
              LittleFS.usedBytes(),
              LittleFS.totalBytes() - LittleFS.usedBytes());
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  }
  ); 
    
  myGame.begin();

  registerGameRoutes(server, myGame);
  registerTaskRoutes(server, myTask);
  registerWifiRoutes(server, myNetwork);
  registerFileRoutes(server, myFile);
  
  server.serveStatic("/", LittleFS, "/");
  server.begin();

  Serial.println("[Server] HTTP Server berjalan di latar belakang.");

/* Initial debugging
 * Probing the system to test its fundamental
 */

  // myTask.printTasks(); Uncomment this line to list available FreeRTOS Tasks.
  // myTask.printCpuLoad(); Uncomment this line to print CPU Load

}

void loop() {
  vTaskDelete(NULL);
}