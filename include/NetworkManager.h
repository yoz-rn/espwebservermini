#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <atomic>
#include <esp_timer.h>

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

    static constexpr uint32_t RECONNECT_BASE_MS = 15000;
    static constexpr uint32_t RECONNECT_MAX_MS =  300000;
    static constexpr uint8_t MAX_RECONNECT_ATTEMPTS  = 5;
    uint8_t _reconnectAttempts = 0;
    std::atomic<uint8_t> _apClientCount{0};
    std::atomic<bool> _manualDisconnectRequested{false};

    uint32_t _reconnectIntervalMs = RECONNECT_BASE_MS;
    esp_timer_handle_t _reconnectTimer = nullptr;

    esp_timer_handle_t _deferredTimer = nullptr;
    enum class DeferredOp { NONE, DISCONNECT, FORGET, RECONNECT };
    DeferredOp _pendingOp = DeferredOp::NONE;
    static void deferredOpCallback(void* arg);
    void runDeferredOp();
    void scheduleDeferred(DeferredOp op);


    static void reconnectTimerCallback(void* arg);
    void attemptReconnect();
    void scheduleReconnect(bool advanceBackOff = true);
    void stopReconnect();

    public:
    NetworkManager(const char* ssid, const char* password);
    
    bool beginAP();
    bool beginSTA(unsigned long timeoutMs = 10000);

    bool saveSTACredentials(const String& ssid, const String& password);
    bool hasSavedCredentials();
    String getSavedSSID();
    bool testSTACredentials(const String& ssid, const String& password, unsigned long timeoutMs = 10000);
    bool isSTAConnected();
    bool forgetSTACredentials();
    bool disconnectSTA();
    bool reconnectSaved();

    bool startSTAProvisioning(const String& ssid, const String& password);
    ProvisioiningStatus getProvisioningStatus();
    String getProvisioningMessage();

    void requestDisconnect();
    void requestForget();
    void requestReconnect();
    
    void notifyInitialSTAFailure();

    void NVSTest();
    
    IPAddress getIP(); // harusnya pake const, nanti cek lagi
    IPAddress getSTAIP();

};