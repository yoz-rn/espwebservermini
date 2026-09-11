#pragma once

#include <WiFi.h>
#include <ESPmDNS.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Preferences.h>

class NetworkManager {
    private:
    const char* _ssid;
    const char* _password;

    const char* _sta_ssid;
    const char* _sta_password;

    Preferences _prefs;

    TaskHandle_t _networkTaskHandle;

    static void taskWrapper(void* _this);
    void taskLoop();

    public:
    NetworkManager(const char* ssid,
                    const char* password,
                    const char* _sta_ssid,
                    const char* _sta_password);
    
    bool beginAP();
    bool beginSTA(unsigned long timeoutMs = 10000);

    bool saveSTACredentials(const String& ssid, const String& password);
    bool hasSavedCredentials();
    bool testSTACredentials(const String& ssid, const String& password, unsigned long timeoutMs = 10000);

    void NVSTest();
    
    IPAddress getIP(); // harusnya pake const, nanti cek lagi
    IPAddress getSTAIP();

};