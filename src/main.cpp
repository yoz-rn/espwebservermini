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

TaskHandle_t g_h2;

void dummyLoop(void* param) {
  for (;;) vTaskDelay(pdMS_TO_TICKS(1000));
}

void dummyLoopSelfDelete(void* param) {
  vTaskDelay(pdMS_TO_TICKS(30000));
  vTaskDelete(NULL);
}

void checkerTask(void* param) {
  vTaskDelay(pdMS_TO_TICKS(40000));   // > 1 detik delay DummyB, beri jeda aman
  char tag[TaskRegistry::TAG_LEN];
  bool stillThere = TaskRegistry::lookup(g_h2, tag, sizeof(tag));
  Serial.printf("[test] DummyB masih di registry? %d\n", stillThere);
  vTaskDelete(NULL);
}

void setup() {
  Serial.begin(115200);
  
  // TaskHandle_t h1;
  // TaskRegistry::monitoredTaskCreate(dummyLoop, "DummyA", 2048, nullptr, 1, &h1, "dummy-a");
  // TaskRegistry::monitoredTaskCreate(dummyLoopSelfDelete, "DummyB", 2048, nullptr, 1, &g_h2, "dummy-b", 1);
  // xTaskCreate(checkerTask, "Checker", 2048, nullptr, 1, nullptr);
  
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
  else myNetwork.notifyInitialSTAFailure();

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