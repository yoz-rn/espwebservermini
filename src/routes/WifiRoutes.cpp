#include "WifiRoutes.h"
#include "ResponseHelper.h"

using PStatus = NetworkManager::ProvisioiningStatus;

static String statusToString( PStatus status) {
    switch (status) {
        case PStatus::IDLE: return "IDLE" ;
        case PStatus::TESTING: return "TESTING";
        case PStatus::SUCCESS: return "SUCCESS";
        case PStatus::FAILED: return "FAILED";
        }
    return "hah?";
    
}

void registerWifiRoutes(AsyncWebServer& server, NetworkManager& networkManager) {
    server.on("/api/wifi-config", HTTP_POST, WIFI_SERVER {
        if (!request->hasParam("ssid", true) || !request->hasParam("password", true)) {
            request->send(400, APP_JSON, buildStatusJson(false, "SSID atau password tidak ada"));
            return;
        }
        
        const AsyncWebParameter* ssidParam = request->getParam("ssid", true);
        const AsyncWebParameter* passwordParam = request->getParam("password", true);

        if (ssidParam == nullptr || passwordParam == nullptr) {
            request->send(400, APP_JSON, buildStatusJson(false, "Gagal membaca SSID/Password"));
            return;
        }

        String ssid = ssidParam->value();
        String password = passwordParam->value();

        bool started = networkManager.startSTAProvisioning(ssid, password);

        started ? request->send(200, APP_JSON, buildStatusJson(true, "proses validasi dimulai"))
                : request->send(400, APP_JSON, buildStatusJson(false, "Proses provisioning lain sedang berjalan"));
    });

    server.on("/api/wifi-status", HTTP_GET, WIFI_SERVER {
        JsonDocument doc;
        doc["status"] = statusToString(networkManager.getProvisioningStatus());
        doc["message"] = networkManager.getProvisioningMessage();

        bool staConnected = networkManager.isSTAConnected();
        doc["staConnected"] = staConnected;
        doc["staIP"] = staConnected ? networkManager.getSTAIP().toString() : "";
        doc["savedSSID"] = networkManager.getSavedSSID();

        String output;
        serializeJson(doc, output);

        request->send(200, APP_JSON, output);
    });

    server.on("/api/wifi-forget", HTTP_POST, [&networkManager](AsyncWebServerRequest *request) {
        networkManager.requestForget();
        request->send(200, APP_JSON, buildStatusJson(true, "Menghapus kredensial..."));
    });

    server.on("/api/wifi-disconnect", HTTP_POST, [&networkManager](AsyncWebServerRequest *request) {
        networkManager.requestDisconnect();
        request->send(200, APP_JSON, buildStatusJson(true, "Memutuskan koneksi..."));
    });

    server.on("/api/wifi-reconnect", HTTP_POST, [&networkManager](AsyncWebServerRequest *request) {
        networkManager.requestReconnect();
        request->send(200, APP_JSON, buildStatusJson(true, "Mencoba menyambungkan kembali"));
    });
}