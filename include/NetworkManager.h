#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <Preferences.h>

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

    Preferences _prefs;

    TaskHandle_t _networkTaskHandle;

    static void taskWrapper(void* _this);
    void taskLoop();

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