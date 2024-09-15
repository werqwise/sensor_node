#include "transparent_serial.h"

SerialMqttBridge::SerialMqttBridge(HardwareSerial &serial, PubSubClient &mqttClient, String topic)
    : _serial(serial), _mqttClient(mqttClient), _topic(topic), _mqttReady(false) {
}

void SerialMqttBridge::begin(unsigned long baudrate) {
    _serial.begin(baudrate);
}

void SerialMqttBridge::end() {
    _serial.end();
}

size_t SerialMqttBridge::write(uint8_t character) {
    // Output to Serial Monitor
    _serial.write(character);

    // Append character to buffer
    _bufferedData += (char)character;

    // If MQTT is ready, publish the character
    if (_mqttReady) {
        char buf[2] = { (char)character, '\0' };
        _mqttClient.publish(_topic.c_str(), buf);
    }

    return 1;
}

size_t SerialMqttBridge::write(const uint8_t *buffer, size_t size) {
    // Output to Serial Monitor
    _serial.write(buffer, size);

    // Append buffer to the buffered data
    _bufferedData += String((const char*)buffer, size);

    // If MQTT is ready, publish the data
    if (_mqttReady) {
        _mqttClient.publish(_topic.c_str(), buffer, size);
    }

    return size;
}

void SerialMqttBridge::loop() {
    // Check if MQTT client is connected and was not ready before
    if (!_mqttReady && _mqttClient.connected()) {
        _mqttReady = true;

        // Publish any buffered data
        if (_bufferedData.length() > 0) {
            _mqttClient.publish(_topic.c_str(), _bufferedData.c_str());
            _bufferedData = "";  // Clear the buffer
        }
    }
}