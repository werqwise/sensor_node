#include <Arduino.h>
#include <map>
#include <string>
#include <functional>
#include <PubSubClient.h>

class SensorManager
{
public:
    // Define a map to keep track of sensor setup statuses
    std::map<String, bool> sensorStatus;

    // Method to setup sensors with retries
    void auto_setup(const String &sensorName, std::function<bool()> setupFunction, int maxAttempts, int delaySeconds)
    {
        bool setupSuccess = false;
        for (int attempt = 1; attempt <= maxAttempts; ++attempt)
        {
            if (setupFunction())
            {
                setupSuccess = true;
                break;
            }
            delay(delaySeconds * 1000); // Delay in milliseconds
        }

        // Store the setup success or failure
        sensorStatus[sensorName] = setupSuccess;

        // Optionally, log the result to the serial monitor
        Serial.print("Setup ");
        Serial.print(sensorName);
        Serial.print(" ");
        Serial.println(setupSuccess ? "succeeded." : "failed.");
    }

    // Method to check if a sensor setup failed
    bool setup_failed(const String &sensorName)
    {
        if (sensorStatus.find(sensorName) != sensorStatus.end())
        {
            return !sensorStatus[sensorName];
        }
        // If the sensor name is not found, we consider it a failure
        return true;
    }
};
class InterceptSerial : public Stream
{
public:
    InterceptSerial(HardwareSerial &originalSerial, PubSubClient &client)
        : originalSerial(originalSerial), mqttClient(client), logTopic("") {}

    void setTopic(const char *topic)
    {
        logTopic = topic;
    }

    size_t write(uint8_t data) override
    {
        buffer += (char)data;
        if (data == '\n' && logTopic.length() > 0)
        {
            mqttClient.publish(logTopic.c_str(), buffer.c_str());
            buffer = "";
        }
        return originalSerial.write(data);
    }

    size_t write(const uint8_t *buffer, size_t size) override
    {
        for (size_t i = 0; i < size; ++i)
        {
            write(buffer[i]);
        }
        return size;
    }

    int available() override
    {
        return originalSerial.available();
    }

    int read() override
    {
        return originalSerial.read();
    }

    int peek() override
    {
        return originalSerial.peek();
    }

    void flush() override
    {
        originalSerial.flush();
    }

private:
    HardwareSerial &originalSerial;
    PubSubClient &mqttClient;
    String logTopic;
    String buffer;
};

// void setup() {
//     Serial.begin(9600);
//     SensorManager sensors;

//     // Setup sensors with retries
//     sensors.auto_setup("BME680", setup_fn1, 5, 1);
//     sensors.auto_setup("MIC", setup_fn2, 5, 1);
// }

// Serial interceptor
// InterceptSerial CustomSerial(MySerial, mqttClient);
// void setup() {
//     MySerial.begin(115200);
//     mqttClient.setServer("mqtt_server_address", 1883);
//     mqttClient.connect("mqtt_client_id");

//     // Set the topic dynamically in the setup function
//     CustomSerial.setTopic("log/topic");

//     // Redirect the global Serial object to our custom serial
//     Serial = CustomSerial;

//     // Example usage
//     Serial.println("Hello, this is a test message!");
// }