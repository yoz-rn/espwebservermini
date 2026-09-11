#include "WifiRoutes.h"
#include "ArduinoJson.h"

// Nanti pindahin biar reusable di modul routes lain
static String buildStatusJson(bool success, const String& message) {
    JsonDocument doc;
    doc["success"] = success;
    doc["message"] = message;

    String output;
    serializeJson(doc, output);
    return output;
}

static void wifiTestTask(void* pvParameters) {
    WifiTestParams* params = static_cast<WifiTestParams*>(pvParameters);

    Serial.printf("[WifiTest] Mulai validasi SSID: %s\n", params->ssid.c_str());

    /* CATATAN RISIKO: request->send() di bawah ini mengasumsikan koneksi client
     * masih hidup. Kalau user menutup tab/browser SEBELUM testSTACredentials()
     * selesai (proses ini bisa makan waktu s/d beberapa detik karena blocking
     * WiFi.begin() + timeout), 'request' ini jadi menunjuk ke koneksi yang
     * sudah mati. Per pengamatan awal, ESPAsyncWebServer menangani ini dengan
     * gagal diam-diam (tidak crash) - TAPI ini belum diuji secara eksplisit.
     * Kalau suatu saat ESP32 crash/restart yang berkorelasi dengan user
     * menutup halaman provisioning di tengah proses, MULAI DEBUG DARI SINI.
     */

    bool success = params->networkManager->testSTACredentials(params->ssid, params->password);
    if (success) {
        params->networkManager->saveSTACredentials(params->ssid, params->password);
        params->request->send(200,
                              "application/json",
                              buildStatusJson(true, "Sukses Terhubung"));
    }
    else
        params->request->send(200,
                              "application/json",
                              buildStatusJson(false, "Gagal terhubung, cek SSID/password"));
    
    Serial.println("[WifiTest] Task selesai, cleanup.");

    delete params;
    vTaskDelete(NULL);
}

void registerWifiRoutes(AsyncWebServer& server, NetworkManager& NetworkManager) {
    server.on("/api/wifi-config", HTTP_POST, [&NetworkManager](AsyncWebServerRequest *request) {
        if (!request->hasParam("ssid", true) || !request->hasParam("password", true)) {
            request->send(400,
                          "application/json",
                          buildStatusJson(false, "SSID/Password NULL"));
            return;
        }
        WifiTestParams* params = new WifiTestParams {
            request,
            &NetworkManager,
            request->getParam("ssid", true)->value(),
            request->getParam("password", true)->value()
        };

        xTaskCreatePinnedToCore(wifiTestTask,
                                "WifiTestTask",
                                4096,
                                params,
                                1,
                                NULL,
                                1);
    });
}