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


 /*
 struct Snap { TaskStatus_t* t; UBaseType_t n; uint32_t total; };
 
 static bool takeSnap(Snap& s) {
   s.n = uxTaskGetNumberOfTasks();
   s.t = (TaskStatus_t*)pvPortMalloc(s.n * sizeof(TaskStatus_t));
   if (!s.t) return false;
   s.n = uxTaskGetSystemState(s.t, s.n, &s.total);
   return true;
 }
 
 void printCpuLoad() {
   Snap a, b;
   if (!takeSnap(a)) return;
   delay(1000);
   if (!takeSnap(b)) {vPortFree(a.t); return;}
 
   uint32_t dTotal = b.total - a.total;
   float sum = 0;
   for (UBaseType_t i = 0; i < b.n; i++) {
     for (UBaseType_t j = 0; j < b.n; j++) {
       if (a.t[j].xTaskNumber == b.t[i].xTaskNumber) {
         uint32_t dTask = b.t[i].ulRunTimeCounter - a.t[j].ulRunTimeCounter;
         float pct = 100.0f * dTask / dTotal;
         sum += pct;
         Serial.printf("%-16s %6.2f%%\n", b.t[i].pcTaskName, pct);
         break;
       }
     }
   }
   Serial.printf("SUM: %.2f%% (dTotal=%lu)\n", sum, (unsigned long)dTotal);
   vPortFree(a.t);
   vPortFree(b.t);
 }
 
 void printTasks() {
   UBaseType_t n = uxTaskGetNumberOfTasks();
   TaskStatus_t* arr = (TaskStatus_t*)pvPortMalloc(n * sizeof(TaskStatus_t));
   if (!arr) return;
 
   uint32_t totalRuntime;
   n = uxTaskGetSystemState(arr, n, &totalRuntime);
 
   for (UBaseType_t i = 0; i < n; i++) {
         Serial.printf("%-16s state=%d prio=%u stackFree=%u",
                   arr[i].pcTaskName,
                   (int)arr[i].eCurrentState,
                   (unsigned)arr[i].uxCurrentPriority,
                   (unsigned)arr[i].usStackHighWaterMark);
 #if CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
     Serial.printf(" core=%d", (int)arr[i].xCoreID);
 #endif
     Serial.println();
   }
   vPortFree(arr);
 }
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
/*
printTasks();
printCpuLoad();
*/


}

void loop() {
  vTaskDelete(NULL);
}