#ifndef ESP32_I2C_COMM_H
#define ESP32_I2C_COMM_H

#include <Arduino.h>
#include <Wire.h>

class ESP32I2CComm
{
public:
    ESP32I2CComm();
    bool begin(int address, void (*callback)(String), bool isMaster = false);
    bool send(String message, int address);
    void update();
    void requestToSend(String message); // New function for slave

private:
    static void receiveEvent(int numBytes);
    static void requestEvent();
    static void (*_callback)(String);
    static String _receivedMessage;
    static String _messageToSend;
    static bool _messageReceived;
    static bool _hasMessageToSend;
    bool _isMaster;
    int _address;
};

ESP32I2CComm::ESP32I2CComm() : _isMaster(false), _address(0) {}

void (*ESP32I2CComm::_callback)(String) = nullptr;
String ESP32I2CComm::_receivedMessage = "";
String ESP32I2CComm::_messageToSend = "";
bool ESP32I2CComm::_messageReceived = false;
bool ESP32I2CComm::_hasMessageToSend = false;

bool ESP32I2CComm::begin(int address, void (*callback)(String), bool isMaster)
{
    _isMaster = isMaster;
    _address = address;
    if (_isMaster)
    {
        Wire.begin(); // Master mode
    }
    else
    {
        if (address < 0x03 || address > 0x77)
        {
            return false; // Invalid address for slave
        }
        Wire.begin(address); // Slave mode
        Wire.onReceive(receiveEvent);
        Wire.onRequest(requestEvent);
    }
    _callback = callback;
    return true;
}

bool ESP32I2CComm::send(String message, int address)
{
    Wire.beginTransmission(address);
    Wire.write((const uint8_t *)message.c_str(), message.length());
    return (Wire.endTransmission() == 0); // Returns true if successful
}

void ESP32I2CComm::update()
{
    if (_messageReceived && _callback)
    {
        _callback(_receivedMessage);
        _messageReceived = false;
        _receivedMessage = "";
    }

    if (_isMaster)
    {
        // Master checks if slave has data to send
        Wire.requestFrom(_address, 1);
        if (Wire.available())
        {
            char c = Wire.read();
            if (c == '!')
            {                                   // Slave has data
                Wire.requestFrom(_address, 32); // Request up to 32 bytes
                String message = "";
                while (Wire.available())
                {
                    message += (char)Wire.read();
                }
                if (_callback)
                {
                    _callback(message);
                }
            }
        }
    }
}

void ESP32I2CComm::requestToSend(String message)
{
    if (!_isMaster)
    {
        _messageToSend = message;
        _hasMessageToSend = true;
    }
}

void ESP32I2CComm::receiveEvent(int numBytes)
{
    _receivedMessage = "";
    while (Wire.available())
    {
        char c = Wire.read();
        _receivedMessage += c;
    }
    _messageReceived = true;
}

void ESP32I2CComm::requestEvent()
{
    if (_hasMessageToSend)
    {
        Wire.write((const uint8_t *)_messageToSend.c_str(), _messageToSend.length());
        _hasMessageToSend = false;
        _messageToSend = "";
    }
    else
    {
        Wire.write(' '); // No data to send
    }
}

#endif //ESP32_I2C_COMM_H
/*
MASTER EXAMPLE
#include "ESP32I2CComm.h"

ESP32I2CComm i2cComm;
const int SLAVE_ADDRESS = 0x55;

void onReceive(String message) {
  Serial.println("Received from slave: " + message);
}

void setup() {
  Serial.begin(115200);
  if (!i2cComm.begin(SLAVE_ADDRESS, onReceive, true)) {
    Serial.println("Failed to initialize I2C master!");
    while (1);
  }
}

void loop() {
  static unsigned long lastSendTime = 0;
  if (millis() - lastSendTime > 5000) {
    if (i2cComm.send("Hello from Master!", SLAVE_ADDRESS)) {
      Serial.println("Message sent to slave");
    } else {
      Serial.println("Failed to send message to slave");
    }
    lastSendTime = millis();
  }
  
  i2cComm.update();  // Check for messages from slave
}
*/

/*
SLAVE EXAMPLE
#include "ESP32I2CComm.h"

ESP32I2CComm i2cComm;
const int SLAVE_ADDRESS = 0x55;

void onReceive(String message) {
  Serial.println("Received from master: " + message);
  // Respond to the master
  i2cComm.requestToSend("Hello back from Slave!");
}

void setup() {
  Serial.begin(115200);
  if (!i2cComm.begin(SLAVE_ADDRESS, onReceive)) {
    Serial.println("Failed to initialize I2C slave!");
    while (1);
  }
}

void loop() {
  i2cComm.update();
  
  // You can also send data periodically if needed
  static unsigned long lastSendTime = 0;
  if (millis() - lastSendTime > 10000) {
    i2cComm.requestToSend("Periodic message from Slave");
    lastSendTime = millis();
  }
}
*/