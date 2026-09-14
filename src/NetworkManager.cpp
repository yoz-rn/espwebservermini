/* THE MOST READABLE PIECE OF C++ CODE
 * YOU WILL HAVE EVER SEEN
 * (I'M GOING CLINICALLY INSANE)
 */

#include "NetworkManager.h"

struct ProvisioningTaskParams {
    NetworkManager* instance;
    String ssid;
    String password;
};

NetworkManager::NetworkManager(const char* ssid, const char* password, const char* sta_ssid, const char* sta_password) {
    _ssid = ssid;
    _password = password;
    _sta_ssid = sta_ssid;
    _sta_password = sta_password;
    _networkTaskHandle = NULL;
    _provisioningMutex = xSemaphoreCreateMutex();
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

    WiFi.mode(WIFI_AP_STA);
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

    _prefs.begin("wifi-config", true); // read-0nly
    String savedSsid = _prefs.getString("ssid", "");
    String savedPassword = _prefs.getString("password", "");
    _prefs.end();

    if (!testSTACredentials(savedSsid, savedPassword, timeoutMs)) {
        Serial.println("STA gagal connect ke router");
        return false;
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

bool NetworkManager::hasSavedCredentials() {
    _prefs.begin("wifi-config", true); // read-0nly
    String savedSsid = _prefs.getString("ssid", "");
    _prefs.end();

    return savedSsid.length() > 0;
}

bool NetworkManager::saveSTACredentials(const String& ssid, const String& password) {
    _prefs.begin("wifi-config", false); //read-write
    bool okSsid = _prefs.putString("ssid", ssid);
    bool okPassword = _prefs.putString("password", password);
    _prefs.end();

    return okSsid && okPassword;
}

bool NetworkManager::testSTACredentials(const String& ssid, const String& password, unsigned long timeoutMs) {
    if (ssid.length() == 0) return false;

    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long startAttempt = millis();
    while(WiFi.status() != WL_CONNECTED) {
        if (millis() - startAttempt > timeoutMs) {
            WiFi.disconnect(); return false;
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    return true;
}

bool NetworkManager::startSTAProvisioning(const String& ssid, const String& password) {
    if (_provisioningStatus == ProvisioiningStatus::TESTING) {
        Serial.println("[Provisioning] Sudah ada proses berjalan, tolak request baru");
        return false;
    }

    xSemaphoreTake(_provisioningMutex, portMAX_DELAY);
    _provisioningStatus = ProvisioiningStatus::TESTING;
    _provisioningMessage = "Mnguji Koneksi";
    xSemaphoreGive(_provisioningMutex);

    ProvisioningTaskParams* params = new ProvisioningTaskParams{this, ssid, password};

    BaseType_t result = xTaskCreatePinnedToCore(
        NetworkManager::provisioningTaskWrapper,
        "ProvisioningTask",
        4096,
        params,
        1,
        NULL,
        1
    );
    if (result != pdPASS) {
        Serial.println("[Provisioning] Gagal membuat task");
        _provisioningStatus = ProvisioiningStatus::FAILED;
        _provisioningMessage = "FAILED TO PROVIDE";
        delete params;

        return false;
    }

    return true;

}

void NetworkManager::provisioningTaskWrapper(void* pvParameters) {
    ProvisioningTaskParams* params = static_cast<ProvisioningTaskParams*>(pvParameters);
    NetworkManager* instance = params->instance;

    Serial.printf("[Provisioning] Mulai validasi SSID: %s\n", params->ssid.c_str());

    bool success = instance->testSTACredentials(params->ssid, params->password);

    xSemaphoreTake(instance->_provisioningMutex, portMAX_DELAY);
    if (success) {
        instance->saveSTACredentials(params->ssid, params->password);
        instance->_provisioningStatus = ProvisioiningStatus::SUCCESS;
        instance->_provisioningMessage = "Berhasil terhubung";
        Serial.println("[Provisioning] Sukses, kredensial disimpan.");
    }
    else {
        instance->_provisioningStatus = ProvisioiningStatus::FAILED;
        instance->_provisioningMessage = "Gagal Terhubung";
        Serial.println("[Provisioning] Gagal.");
    }
    xSemaphoreGive(instance->_provisioningMutex);

    delete params;
    vTaskDelete(NULL);
}


/*
 * GETTER
 * SETTER
 */

IPAddress NetworkManager::getIP() {
    return WiFi.softAPIP();
}

IPAddress NetworkManager::getSTAIP() {
    return WiFi.localIP();
}

NetworkManager::ProvisioiningStatus NetworkManager::getProvisioningStatus() {
    xSemaphoreTake(_provisioningMutex, portMAX_DELAY);
    ProvisioiningStatus result = _provisioningStatus;
    xSemaphoreGive(_provisioningMutex);
    return result;
}

String NetworkManager::getProvisioningMessage() {
    xSemaphoreTake(_provisioningMutex, portMAX_DELAY);
    String result = _provisioningMessage;
    xSemaphoreGive(_provisioningMutex);
    return result;
}

void NetworkManager::NVSTest() {
    // --- NVS TEST ---
  Serial.println("[Test] Cek kredensial tersimpan...");
  Serial.println(hasSavedCredentials() ? "Ada" : "Belum ada");

  Serial.println("[Test] Simpan kredensial dummy...");
  bool saved = saveSTACredentials("TestSSID", "TestPassword123");
  Serial.println(saved ? "Berhasil disimpan" : "Gagal disimpan");

  Serial.println("[Test] Cek ulang kredensial tersimpan...");
  Serial.println(hasSavedCredentials() ? "Ada" : "Belum ada");
  // --- ENDOF NVS TEST ---
}