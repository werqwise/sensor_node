#include <Arduino.h>
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
    return true;
}

int PirSensor::sampleValue()
{
    unsigned long now = millis();
    if (now - lastSample >= 500)
    {
        lastSample = now;
        int newValue = digitalRead(pin); // Changed to int for clarity with digitalRead
        samples[sampleIndex] = newValue;
        sampleIndex = (sampleIndex + 1) % samplesTotal;
        float lastValue = readValue();
        if (newValue == 1 && lastValue < 1)
            return 1; // Adjusted to simpler logic
        if (newValue == 0 && lastValue > 0)
            return 0;
        return -1;
    }
    return -1;
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
#define MOTION_SENSOR_PIN 3

PirSensor motion = PirSensor(MOTION_SENSOR_PIN, 2, false, false);

int setup_pir()
{
    int result = motion.begin();
    SMB.print("PIR sensor setup result: ");
    SMB.println(result);
    // Assuming begin() returns a meaningful result you can check
    if (result != 1)
    {
        SMB.println("Error initializing PIR sensor!");
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