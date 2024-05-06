#include <Arduino.h>
#include <PirSensor.h>

#define PUBLISH_INTERVAL 15
#define MOTION_SENSOR_PIN 39

PirSensor motion = PirSensor(MOTION_SENSOR_PIN, 2, false, false);

int setup_pir()
{
    return motion.begin();
    return 1;
}

int get_pir()
{
    int motionStateChange = motion.sampleValue();
    if (motionStateChange >= 0)
    {
        return motionStateChange;
    }
    else{
        return 0;
    }
}