#include <Arduino.h>
const int MQ135_SENSOR_PIN = 35;

int sensitivity = 200; // Adjust this value based on your calibration

const int sleepInterval = 3600; // Deep sleep for 1 hour (in seconds)

int get_aqi_mq135()
{

    int sensor_value = analogRead(MQ135_SENSOR_PIN);
    int air_quality = sensor_value * sensitivity / 1023;
    return air_quality;
}
