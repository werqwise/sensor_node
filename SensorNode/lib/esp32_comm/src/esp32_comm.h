#ifndef ESP32_COMM_LIB_H
#define ESP32_COMM_LIB_H

#include <Arduino.h>
#include <WiFi.h>

class ESP32Comm
{
public:
    typedef void (*DataCallback)(String);

    ESP32Comm(bool isMaster, const char *ssid, const char *password, int port = 8888);
    void begin();
    void loop();
    bool send(String message);
    void setDataCallback(DataCallback callback);
    void disconnect();

private:
    bool _isMaster;
    const char *_ssid;
    const char *_password;
    int _port;
    WiFiServer *_server;
    WiFiClient _client;
    DataCallback _dataCallback;
    unsigned long _lastReconnectAttempt;

    void setupWiFi();
    void handleConnection();
    void reconnect();
};

ESP32Comm::ESP32Comm(bool isMaster, const char *ssid, const char *password, int port)
    : _isMaster(isMaster), _ssid(ssid), _password(password), _port(port), _server(nullptr), _dataCallback(nullptr), _lastReconnectAttempt(0) {}

void ESP32Comm::begin()
{
    setupWiFi();
    if (_isMaster)
    {
        _server = new WiFiServer(_port);
        _server->begin();
    }
}
void ESP32Comm::loop()
{
    if (_isMaster)
    {
        handleConnection();
    }
    else
    {
        if (!_client.connected())
        {
            reconnect();
        }
    }

    if (_client.connected())
    {
        while (_client.available())
        {
            String data = _client.readStringUntil('\n');
            if (_dataCallback)
            {
                _dataCallback(data);
            }
        }
    }
}
void ESP32Comm::disconnect()
{
    if (_client)
    {
        _client.stop();
    }
    if (_server)
    {
        _server->close();
        delete _server;
        _server = nullptr;
    }
}
bool ESP32Comm::send(String message)
{
    if (_client.connected())
    {
        return _client.println(message);
    }
    Serial.println("Cannot send message: client not connected");
    return false;
}

void ESP32Comm::setDataCallback(DataCallback callback)
{
    _dataCallback = callback;
}

void ESP32Comm::setupWiFi()
{
    WiFi.mode(_isMaster ? WIFI_AP_STA : WIFI_STA);
    if (_isMaster)
    {
        WiFi.softAP(_ssid, _password);
    }
    else
    {
        WiFi.begin(_ssid, _password);
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
        }
    }
}

void ESP32Comm::handleConnection()
{
    if (!_client || !_client.connected())
    {
        _client = _server->available();
    }
}

void ESP32Comm::reconnect()
{
    unsigned long now = millis();
    if (now - _lastReconnectAttempt > 5000)
    {
        _lastReconnectAttempt = now;
        Serial.println("Attempting to reconnect...");
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("WiFi not connected. Reconnecting to WiFi...");
            WiFi.begin(_ssid, _password);
            delay(5000); // Wait for WiFi connection
        }
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.print("Connecting to server at ");
            Serial.println(WiFi.gatewayIP());
            if (_client.connect(WiFi.gatewayIP(), _port))
            {
                Serial.println("Connected to server");
                _lastReconnectAttempt = 0;
            }
            else
            {
                Serial.println("Failed to connect to server");
            }
        }
        else
        {
            Serial.println("Failed to connect to WiFi");
        }
    }
}
#endif // ESP32_COMM_LIB_H