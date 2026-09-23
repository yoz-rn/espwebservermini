/* THE MOST READABLE PIECE OF C++ CODE
 * YOU WILL HAVE EVER SEEN
 * (I'M GOING CLINICALLY INSANE)
 * 
 * WAIT TILL YOU SEE THE TASKMANAGER MODULE
 */

#include <esp_log.h>
#include "NetworkManager.h"

static const char* TAG = "network";

constexpr uint32_t NetworkManager::RECONNECT_BASE_MS;
constexpr uint32_t NetworkManager::RECONNECT_MAX_MS;

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
            _apClientCount++;
            ESP_LOGI(TAG, "AP: Client connected " MACSTR, MAC2STR(info.wifi_ap_staconnected.mac));
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            if (_apClientCount > 0) _apClientCount--;
            ESP_LOGI(TAG, "AP: Client disconnected " MACSTR, MAC2STR(info.wifi_ap_stadisconnected.mac));
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            _staConnected = true;
            _reconnectIntervalMs = RECONNECT_BASE_MS;
            _reconnectAttempts = 0;
            stopReconnect();
            ESP_LOGI(TAG, "STA tersambung, IP: " IPSTR, IP2STR(&info.got_ip.ip_info.ip));
            if (MDNS.begin("esp32")) ESP_LOGI(TAG, "mDNS aktif: http://esp32.local");
            else                     ESP_LOGE(TAG, "mDNS gagal start");
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            if (_staConnected) {                 // baru saja terputus dari kondisi tersambung
                _staConnected = false;
                MDNS.end();
                ESP_LOGW(TAG, "STA disconnected (reason %d)", info.wifi_sta_disconnected.reason);

                if (_manualDisconnectRequested) {
                    _manualDisconnectRequested = false;
                    ESP_LOGI(TAG, "Manually disconnected.");
                }

                else if (hasSavedCredentials()) {
                    _reconnectAttempts = 0;
                    _reconnectIntervalMs = RECONNECT_BASE_MS;
                    scheduleReconnect();
                }
            } else {                             // percobaan gagal: tidak perlu berisik
                ESP_LOGD(TAG, "STA failed to connect (reason %d)", info.wifi_sta_disconnected.reason);
            }
            break;
        default:
            break;
    }
}

String NetworkManager::getSavedSSID() {
    _prefs.begin("wifi-config", true);
    String ssid = _prefs.getString("ssid", "");
    _prefs.end();
    return ssid;
}

bool NetworkManager::reconnectSaved() {
    if (_staConnected) return false;
    if (!hasSavedCredentials()) return false;

    stopReconnect();
    _reconnectAttempts = 0;
    _reconnectIntervalMs = RECONNECT_BASE_MS;

    _prefs.begin("wifi-config", true);
    String ssid = _prefs.getString("ssid", "");
    String password = _prefs.getString("password", "");
    _prefs.end();

    ESP_LOGI(TAG, "Reconnect manual triggered by user: \"%s\"", ssid.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());
    return true;
}

bool NetworkManager::forgetSTACredentials() {
    if (_staConnected) {
        _manualDisconnectRequested = true;
        WiFi.disconnect();
    }

    stopReconnect();
    _reconnectAttempts = 0;
    _reconnectIntervalMs = RECONNECT_BASE_MS;
    
    _prefs.begin("wifi-config", false);
    bool ok = _prefs.remove("ssid") && _prefs.remove("password");
    _prefs.end();

    if (ok) {
        _reconnectAttempts = 0;
        _reconnectIntervalMs = RECONNECT_BASE_MS;
        stopReconnect();
        ESP_LOGI(TAG, "STA Credential deleted!");
    }
    return ok;
}

bool NetworkManager::disconnectSTA() {
    if (!_staConnected) return false;

    _manualDisconnectRequested = true;
    stopReconnect();
    WiFi.disconnect();
    ESP_LOGI(TAG, "STA Disconnected manual by user");
    return true;
}

void NetworkManager::scheduleReconnect(bool advanceBackoff) {
    if (_reconnectTimer == nullptr) {
        esp_timer_create_args_t args = {};
        args.callback = &NetworkManager::reconnectTimerCallback;
        args.arg = this;
        args.name = "reconnect";
        esp_timer_create(&args, &_reconnectTimer);
    }

    ESP_LOGI(TAG, "Reconnect scheduled in %u seconds", (unsigned)(_reconnectIntervalMs / 1000));
    esp_timer_start_once(_reconnectTimer, (uint64_t)_reconnectIntervalMs * 1000); // esp_timer pakai microsecond

    // siapkan interval berikutnya buat percobaan selanjutnya, di-cap di RECONNECT_MAX_MS
    if (advanceBackoff) _reconnectIntervalMs = min(_reconnectIntervalMs * 2, RECONNECT_MAX_MS);
}

void NetworkManager::reconnectTimerCallback(void* arg) {
    static_cast<NetworkManager*>(arg)->attemptReconnect();
}

void NetworkManager::attemptReconnect() {
    if (_staConnected) return;
    if (!hasSavedCredentials()) return;

    if (_apClientCount > 0) {
        ESP_LOGI(TAG, "%u client currently active, holding reconnect", (unsigned)_apClientCount);
        scheduleReconnect(false);
        return;
    }

    if (_reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
        ESP_LOGW(TAG, "Giving up after %u attempts for this episode", MAX_RECONNECT_ATTEMPTS);
        return;
    }
    _reconnectAttempts++;

    _prefs.begin("wifi-config", true);
    String ssid = _prefs.getString("ssid", "");
    String password = _prefs.getString("password", "");
    _prefs.end();

    ESP_LOGI(TAG, "Auto-reconnect counter: %u/%u | Trying: \"%s\"",
                _reconnectAttempts, MAX_RECONNECT_ATTEMPTS, ssid.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());

    scheduleReconnect(true);
}

void NetworkManager::stopReconnect() {
    if (_reconnectTimer != nullptr) esp_timer_stop(_reconnectTimer);
}

void NetworkManager::notifyInitialSTAFailure() {
    if (_staConnected) return;
    if (!hasSavedCredentials()) return;

    ESP_LOGI(TAG, "STA failed to connect at boot, initializing backoff cycle");
    _reconnectAttempts = 0;
    _reconnectIntervalMs = RECONNECT_BASE_MS;

    scheduleReconnect();
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

bool NetworkManager::isSTAConnected() {
    return _staConnected;
}

// Early testing
void NetworkManager::NVSTest() {
        
    Serial.println("[Test] Cek kredensial tersimpan...");
    Serial.println(hasSavedCredentials() ? "Ada" : "Belum ada");

    Serial.println("[Test] Simpan kredensial dummy...");
    bool saved = saveSTACredentials("TestSSID", "TestPassword123");
    Serial.println(saved ? "Berhasil disimpan" : "Gagal disimpan");

    Serial.println("[Test] Cek ulang kredensial tersimpan...");
    Serial.println(hasSavedCredentials() ? "Ada" : "Belum ada");
  
}

/* LWIP TCP Handler Block
 * Slighlty over-engineered
 * But hey, nevermind
 */ 

void NetworkManager::scheduleDeferred(DeferredOp op) {
    _pendingOp = op;
    if (_deferredTimer == nullptr) {
        esp_timer_create_args_t args = {};
        args.callback = &NetworkManager::deferredOpCallback;
        args.arg = this;
        args.name = "deferred-op";
        esp_timer_create(&args, &_deferredTimer);
    }
    esp_timer_start_once(_deferredTimer, 300000);
}

void NetworkManager::requestDisconnect() { scheduleDeferred(DeferredOp::DISCONNECT); }
void NetworkManager::requestForget() { scheduleDeferred(DeferredOp::FORGET); }
void NetworkManager::requestReconnect() { scheduleDeferred(DeferredOp::RECONNECT); }

void NetworkManager::deferredOpCallback(void* arg) {
    static_cast<NetworkManager*>(arg)->runDeferredOp();
}

void NetworkManager::runDeferredOp() {
    DeferredOp op = _pendingOp;
    _pendingOp = DeferredOp::NONE;
    switch (op) {
        case DeferredOp::DISCONNECT: disconnectSTA(); break;
        case DeferredOp::FORGET: forgetSTACredentials(); break;
        case DeferredOp::RECONNECT: reconnectSaved(); break;
        default: break;
    }
}