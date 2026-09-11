#include "NetworkManager.h"

NetworkManager::NetworkManager(const char* ssid, const char* password, const char* sta_ssid, const char* sta_password) {
    _ssid = ssid;
    _password = password;
    _sta_ssid = sta_ssid;
    _sta_password = sta_password;
    _networkTaskHandle = NULL;
}

bool NetworkManager::beginAP() {
    Serial.println("AP Initializing...");

    IPAddress local_ip(192, 168, 1, 254);
    IPAddress gateway(192, 168, 1, 254);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(local_ip, gateway, subnet);

    if (_networkTaskHandle != NULL) {
        Serial.println("udah jalan");
        return false;
    } 

    if (_ssid == nullptr || strlen(_ssid) == 0) {
        Serial.println("ssid kosong");
        return false;
    }

    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP(_ssid, _password)) {
        Serial.println("softAP gagal");
        return false;
    }

    BaseType_t result = xTaskCreatePinnedToCore(
        NetworkManager::taskWrapper,
        "NetworkTask",
        4096,
        this,
        1,
        &_networkTaskHandle,
        1
    );

    if (result != pdPASS) {
        Serial.println("gagal init task monitor");
        return false;
    }

    Serial.println("Berhasil inisialisasi");
    return true;
}

bool NetworkManager::beginSTA(unsigned long timeoutMs) {
    Serial.println("STA Initializing...");
    if (_sta_ssid == nullptr || strlen(_sta_ssid) == 0) {
        Serial.println("STA ssid kosong, skip");
        return false;
    }
    WiFi.begin(_sta_ssid, _sta_password);

    unsigned long startAttempt = millis();
    while(WiFi.status() != WL_CONNECTED) {
        if (millis() - startAttempt > timeoutMs) {
            Serial.println("STA timeout, gagal connect ke router");
            return false;
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    Serial.print("STA connected, IP: ");
    Serial.println(WiFi.localIP());

    MDNS.begin("esp32") ? Serial.println("mDNS responder aktif: http://esp32.local")
                        : Serial.println("mDNS gagal start");

    return true;

}

void NetworkManager::taskWrapper(void* _this) {
    NetworkManager* instance = static_cast<NetworkManager*>(_this);
    instance->taskLoop();
}

void NetworkManager::taskLoop() {
    for(;;) {
        uint8_t clientCount = WiFi.softAPgetStationNum();
        Serial.printf("[Network Task] Klien terhubung: %d\n", clientCount);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

IPAddress NetworkManager::getIP() {
    return WiFi.softAPIP();
}

IPAddress NetworkManager::getSTAIP() {
    return WiFi.localIP();
}