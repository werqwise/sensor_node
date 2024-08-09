#ifndef ESP32_PROXIMITY_COMM_H
#define ESP32_PROXIMITY_COMM_H

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

class ESP32ProximityComm
{
public:
    ESP32ProximityComm(int rssiThreshold = -70);
    bool begin(void (*onReceiveCallback)(String message));
    void sendBroadcast(String message);
    bool sendMessage(const uint8_t *peerAddress, String message);
    void loop();

private:
    static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
    static void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int len);
    static void (*_onReceiveCallback)(String message);
    int _rssiThreshold;
    static ESP32ProximityComm *instance; // Singleton instance
};

ESP32ProximityComm* ESP32ProximityComm::instance = nullptr;
void (*ESP32ProximityComm::_onReceiveCallback)(String message) = nullptr;

ESP32ProximityComm::ESP32ProximityComm(int rssiThreshold)
    : _rssiThreshold(rssiThreshold)
{
    instance = this;
}

bool ESP32ProximityComm::begin(void (*onReceiveCallback)(String message))
{
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK)
    {
        Serial.println("Error initializing ESP-NOW");
        return false;
    }

    _onReceiveCallback = onReceiveCallback;
    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataRecv);

    return true;
}

void ESP32ProximityComm::sendBroadcast(String message)
{
    const uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_now_send(broadcastAddress, (const uint8_t *)message.c_str(), message.length());
}

bool ESP32ProximityComm::sendMessage(const uint8_t *peerAddress, String message)
{
    return (esp_now_send(peerAddress, (const uint8_t *)message.c_str(), message.length()) == ESP_OK);
}

void ESP32ProximityComm::loop()
{
    // Additional functionality can be added here
}

void ESP32ProximityComm::onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    Serial.print("Last Packet Send Status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void ESP32ProximityComm::onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    int rssi = WiFi.RSSI();

    if (rssi > instance->_rssiThreshold)
    {
        String message = "";
        for (int i = 0; i < len; i++)
        {
            message += (char)data[i];
        }

        if (_onReceiveCallback)
        {
            _onReceiveCallback(message);
        }

        // Add this device to the peer list
        esp_now_peer_info_t peerInfo;
        memcpy(peerInfo.peer_addr, mac_addr, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;

        if (esp_now_add_peer(&peerInfo) != ESP_OK)
        {
            Serial.println("Failed to add peer.");
        }
    }
    else
    {
        Serial.println("Device too far, ignoring message.");
    }
}
#endif // ESP32_PROXIMITY_COMM_H