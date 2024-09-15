#ifndef ESP32_OTA_H
#define ESP32_OTA_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

class ESP32_OTA {
public:
    ESP32_OTA(const char* serverUrl);
    void begin();
    void handle();

private:
    const char* _serverUrl;
    unsigned long _previousMillis = 0;
    const unsigned long _interval = 60000; // Check every minute (adjust as needed)
    void _connectWiFi();
    void _registerDevice();
    void _checkForUpdates();
};

#endif // ESP32_OTA_H