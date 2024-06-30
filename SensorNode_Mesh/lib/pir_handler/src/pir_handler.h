#include <Arduino.h>
// #include <PirSensor.h>
#ifndef PIR_SENSOR_H
#define PIR_SENSOR_H

class PirSensor
{
private:
    bool debug;
    bool verbose;
    int sampleIndex;
    int samplesTotal;
    float *samples;
    uint8_t pin;
    unsigned long lastSample;

public:
    PirSensor(uint8_t sensorPin, int samplesCount, bool debugSetting, bool verboseSetting);
    ~PirSensor();
    bool begin();
    int sampleValue();
    float readValue();
};

#endif // PIR_SENSOR_H

// Implementation of PirSensor class
#include <PirSensor.h>
#include <Math.h>

PirSensor::PirSensor(uint8_t sensorPin, int samplesCount, bool debugSetting, bool verboseSetting)
    : pin(sensorPin), samplesTotal(samplesCount), debug(debugSetting), verbose(verboseSetting), sampleIndex(0), lastSample(0)
{
    samples = new float[samplesCount]();
}

PirSensor::~PirSensor()
{
    delete[] samples;
}

bool PirSensor::begin()
{
    pinMode(pin, INPUT);
    return digitalRead(pin) != -1; // Simple check to see if the pin is operational
}

int PirSensor::sampleValue()
{
    unsigned long now = millis();
    // Throttle sampling to no more than once every 500 milliseconds
    if (now - lastSample >= 500)
    {
        lastSample = now;
        float newValue = digitalRead(pin);
        samples[sampleIndex] = newValue;
        sampleIndex = (sampleIndex + 1) % samplesTotal;
        float lastValue = readValue();
        // Return 1 for motion detected, 0 for no motion, -1 if no change
        return (newValue == 1 && lastValue < 0.5) ? 1 : (newValue == 0 && lastValue > 0.5) ? 0
                                                                                           : -1;
    }
    return -1; // No new sample taken
}

float PirSensor::readValue()
{
    float sum = 0;
    int count = 0;
    for (int i = 0; i < samplesTotal; i++)
    {
        if (samples[i] != -1)
        { // Assuming -1 is an invalid sample
            sum += samples[i];
            count++;
        }
    }
    return count > 0 ? sum / count : -1;
}

#define PUBLISH_INTERVAL 15
#define MOTION_SENSOR_PIN 39

PirSensor motion = PirSensor(MOTION_SENSOR_PIN, 2, false, false);

int setup_pir()
{
    int result = motion.begin();
    Serial.print("PIR sensor setup result: ");
    Serial.println(result);
    // Assuming begin() returns a meaningful result you can check
    if (result != 1)
    {
        Serial.println("Error initializing PIR sensor!");
        return 0;
    }
    else
    {
        return 1;
    }
}

int get_pir()
{
    int motionStateChange = motion.sampleValue();
    if (motionStateChange >= 0)
    {
        return motionStateChange;
    }
    else
    {
        return 0;
    }
}