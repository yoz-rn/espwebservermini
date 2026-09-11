#pragma once

#include <WiFi.h>
#include <ESPmDNS.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class NetworkManager {
    private:
    const char* _ssid;
    const char* _password;

    const char* _sta_ssid;
    const char* _sta_password;

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
    
    IPAddress getIP(); // harusnya pake const, nanti cek lagi
    IPAddress getSTAIP();

};