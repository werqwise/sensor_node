#ifndef ESP32_PROXIMITY_COMM_H
#define ESP32_PROXIMITY_COMM_H

#include <Arduino.h>
#include <WiFi.h>

class ESP32ProximityComm
{
public:
    ESP32ProximityComm(bool isMaster, const char *ssid, const char *password, IPAddress ip = IPAddress(192, 168, 4, 1), int port = 80);
    bool begin(void (*onReceiveCallback)(String message));
    void sendBroadcast(String message);
    void loop();

private:
    static void (*_onReceiveCallback)(String message);
    bool _isMaster;
    const char *_ssid;
    const char *_password;
    IPAddress _ip;
    int _port;
    WiFiServer *_server;
    WiFiClient _client;
    WiFiClient _slaveClient;

    void startMaster();
    void startSlave();
};

void (*ESP32ProximityComm::_onReceiveCallback)(String message) = nullptr;

ESP32ProximityComm::ESP32ProximityComm(bool isMaster, const char *ssid, const char *password, IPAddress ip, int port)
    : _isMaster(isMaster), _ssid(ssid), _password(password), _ip(ip), _port(port), _server(nullptr) {}

bool ESP32ProximityComm::begin(void (*onReceiveCallback)(String message))
{
    _onReceiveCallback = onReceiveCallback;

    if (_isMaster)
    {
        startMaster();
    }
    else
    {
        startSlave();
    }

    return true;
}

void ESP32ProximityComm::startMaster()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(_ssid, _password);

    _server = new WiFiServer(_port);
    _server->begin();

    Serial.println("Master started as Access Point.");
}

void ESP32ProximityComm::startSlave()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Connecting to AP...");
    }

    _slaveClient.connect(_ip, _port);
    Serial.println("Slave connected to Master.");
}

void ESP32ProximityComm::sendBroadcast(String message)
{
    if (_isMaster)
    {
        if (_client && _client.connected())
        {
            _client.print(message);
        }
    }
    else
    {
        if (_slaveClient && _slaveClient.connected())
        {
            _slaveClient.print(message);
        }
    }
}

void ESP32ProximityComm::loop()
{
    if (_isMaster)
    {
        _client = _server->available();
        if (_client && _client.connected())
        {
            if (_client.available())
            {
                String message = _client.readStringUntil('\n');
                if (_onReceiveCallback)
                {
                    _onReceiveCallback(message);
                }
            }
        }
    }
    else
    {
        if (_slaveClient && _slaveClient.connected() && _slaveClient.available())
        {
            String message = _slaveClient.readStringUntil('\n');
            if (_onReceiveCallback)
            {
                _onReceiveCallback(message);
            }
        }
    }
}
#endif // ESP32_PROXIMITY_COMM_H