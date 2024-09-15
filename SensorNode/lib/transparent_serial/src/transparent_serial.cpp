#include "transparent_serial.h"

MqttSerial::MqttSerial(HardwareSerial &serial, PubSubClient &mqttClient, String topic)
    : _serial(serial), _mqttClient(mqttClient), _topic(topic), _mqttReady(false) {
    // Create a mutex
    _mutex = xSemaphoreCreateMutex();
}

void MqttSerial::begin(unsigned long baudrate) {
    _serial.begin(baudrate);
}

void MqttSerial::end() {
    _serial.end();
}

size_t MqttSerial::write(uint8_t character) {
    // Output to Serial Monitor
    _serial.write(character);

    // Acquire mutex
    xSemaphoreTake(_mutex, portMAX_DELAY);

    // Append character to buffer
    _bufferedData += (char)character;

    // If MQTT is ready, publish the character
    if (_mqttReady) {
        char buf[2] = { (char)character, '\0' };
        _mqttClient.publish(_topic.c_str(), buf);
    }

    // Release mutex
    xSemaphoreGive(_mutex);

    return 1;
}

size_t MqttSerial::write(const uint8_t *buffer, size_t size) {
    // Output to Serial Monitor
    _serial.write(buffer, size);

    // Acquire mutex
    xSemaphoreTake(_mutex, portMAX_DELAY);

    // Append buffer to the buffered data
    _bufferedData += String((const char*)buffer, size);

    // If MQTT is ready, publish the data
    if (_mqttReady) {
        _mqttClient.publish(_topic.c_str(), buffer, size);
    }

    // Release mutex
    xSemaphoreGive(_mutex);

    return size;
}

void MqttSerial::loop() {
    // Acquire mutex
    xSemaphoreTake(_mutex, portMAX_DELAY);

    // Check if MQTT client is connected and was not ready before
    if (!_mqttReady && _mqttClient.connected()) {
        _mqttReady = true;

        // Publish any buffered data
        if (_bufferedData.length() > 0) {
            _mqttClient.publish(_topic.c_str(), _bufferedData.c_str());
            _bufferedData = "";  // Clear the buffer
        }
    }

    // Release mutex
    xSemaphoreGive(_mutex);
}