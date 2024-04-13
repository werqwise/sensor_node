#include <Arduino.h>
#include <map>
#include <string>
#include <functional>

class SensorManager {
public:
    // Define a map to keep track of sensor setup statuses
    std::map<String, bool> sensorStatus;

    // Method to setup sensors with retries
    void auto_setup(const String& sensorName, std::function<bool()> setupFunction, int maxAttempts, int delaySeconds) {
        bool setupSuccess = false;

        for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
            if (setupFunction()) {
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
    bool setup_failed(const String& sensorName) {
        if (sensorStatus.find(sensorName) != sensorStatus.end()) {
            return !sensorStatus[sensorName];
        }
        // If the sensor name is not found, we consider it a failure
        return true;
    }
};

// void setup() {
//     Serial.begin(9600);
//     SensorManager sensors;

//     // Setup sensors with retries
//     sensors.auto_setup("BME680", setup_fn1, 5, 1);
//     sensors.auto_setup("MIC", setup_fn2, 5, 1);
// }