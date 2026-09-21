#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <atomic>

class NetworkManager {
    public:
    enum class ProvisioiningStatus {
        IDLE,
        TESTING,
        SUCCESS,
        FAILED
    };
    
    private:
    const char* _ssid;
    const char* _password;

    bool _started = false;
    std::atomic<bool> _staConnected{false};
    void onWifiEvent(arduino_event_id_t event, arduino_event_info_t info);

    Preferences _prefs;
    
    ProvisioiningStatus _provisioningStatus = ProvisioiningStatus::IDLE;
    String _provisioningMessage;
    SemaphoreHandle_t _provisioningMutex;

    static void provisioningTaskWrapper(void* pvParameters);

    public:
    NetworkManager(const char* ssid, const char* password);
    
    bool beginAP();
    bool beginSTA(unsigned long timeoutMs = 10000);

    bool saveSTACredentials(const String& ssid, const String& password);
    bool hasSavedCredentials();
    bool testSTACredentials(const String& ssid, const String& password, unsigned long timeoutMs = 10000);

    bool startSTAProvisioning(const String& ssid, const String& password);
    ProvisioiningStatus getProvisioningStatus();
    String getProvisioningMessage();

    void NVSTest();
    
    IPAddress getIP(); // harusnya pake const, nanti cek lagi
    IPAddress getSTAIP();

};