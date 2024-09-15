#ifndef MQTT_SERIAL_H
#define MQTT_SERIAL_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <PubSubClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class MqttSerial : public Print {
public:
    // Constructor
    MqttSerial(HardwareSerial &serial, PubSubClient &mqttClient, String topic);

    // Override write methods
    virtual size_t write(uint8_t character) override;
    virtual size_t write(const uint8_t *buffer, size_t size) override;

    // Begin and end methods
    void begin(unsigned long baudrate);
    void end();

    // Loop method to handle MQTT readiness and buffer management
    void loop();

private:
    HardwareSerial &_serial;    // Reference to the HardwareSerial object
    PubSubClient &_mqttClient;  // Reference to the MQTT client
    String _topic;              // MQTT topic to publish to
    String _bufferedData;       // Buffer to store data until MQTT is ready
    bool _mqttReady;            // Flag to indicate if MQTT client is ready

    // Mutex for synchronization
    SemaphoreHandle_t _mutex;
};

#endif // MQTT_SERIAL_H