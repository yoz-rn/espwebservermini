/* THE MOST READABLE PIECE OF C++ CODE
 * YOU WILL HAVE EVER SEEN
 * (I'M GOING CLINICALLY INSANE)
 * 
 * WAIT TILL YOU SEE THE TASKMANAGER MODULE
 */

#include <esp_log.h>
#include "NetworkManager.h"

static const char* TAG = "network";

struct ProvisioningTaskParams {
    NetworkManager* instance;
    String ssid;
    String password;
};

NetworkManager::NetworkManager(const char* ssid, const char* password) {
    _ssid = ssid;
    _password = password;
    _provisioningMutex = xSemaphoreCreateMutex();
}

bool NetworkManager::beginAP() {
    if (_started) {
        ESP_LOGW(TAG, "beginAP() dipanggil dua kali, diabaikan");
        return false;
    }
    
    if (_ssid == nullptr || strlen(_ssid) == 0) {
        Serial.println("ssid kosong");
        return false;
    }

    WiFi.onEvent([this](arduino_event_id_t event, arduino_event_info_t info) {
        onWifiEvent(event, info);
    });

    IPAddress local_ip(192, 168, 1, 254);
    IPAddress gateway(192, 168, 1, 254);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(local_ip, gateway, subnet);

    WiFi.mode(WIFI_AP_STA);
    if (!WiFi.softAP(_ssid, _password)) {
        ESP_LOGE(TAG, "SoftAP Failed");
        return false;
    }

    _started = true;
    ESP_LOGI(TAG, "AP Active: %s", _ssid);
    return true;
}

bool NetworkManager::beginSTA(unsigned long timeoutMs) {

    _prefs.begin("wifi-config", true); // read-0nly
    String savedSsid = _prefs.getString("ssid", "");
    String savedPassword = _prefs.getString("password", "");
    _prefs.end();

    if (savedSsid.length() == 0) {
        ESP_LOGI(TAG, "No STA Credentials. Running AP-Only mode");
        return false;
    }

    if (!testSTACredentials(savedSsid, savedPassword, timeoutMs)) {
        ESP_LOGW(TAG, "STA failed to connect to \"%s\"", savedSsid.c_str());
        return false;
    }
        
    return true;

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

void NetworkManager::onWifiEvent(arduino_event_id_t event, arduino_event_info_t info) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            ESP_LOGI(TAG, "AP: klien tersambung " MACSTR, MAC2STR(info.wifi_ap_staconnected.mac));
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            ESP_LOGI(TAG, "AP: klien putus " MACSTR, MAC2STR(info.wifi_ap_stadisconnected.mac));
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            _staConnected = true;
            ESP_LOGI(TAG, "STA tersambung, IP: " IPSTR, IP2STR(&info.got_ip.ip_info.ip));
            if (MDNS.begin("esp32")) ESP_LOGI(TAG, "mDNS aktif: http://esp32.local");
            else                     ESP_LOGE(TAG, "mDNS gagal start");
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            if (_staConnected) {                 // baru saja terputus dari kondisi tersambung
                _staConnected = false;
                MDNS.end();
                ESP_LOGW(TAG, "STA terputus (alasan %d)", info.wifi_sta_disconnected.reason);
            } else {                             // percobaan gagal: tidak perlu berisik
                ESP_LOGD(TAG, "STA gagal menyambung (alasan %d)", info.wifi_sta_disconnected.reason);
            }
            break;
        default:
            break;
    }
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