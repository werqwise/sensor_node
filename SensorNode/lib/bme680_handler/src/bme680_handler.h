#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME680 bme; // I2C

int setup_bme680()
{

    SMB.println(F("BME280 test"));

    unsigned status;

    // default settings
    status = bme.begin();

    if (!status)
    {
        SMB.println("Could not find a valid BME280 sensor, check wiring.");
        return 0;
    }
    else
    {
        // Set up oversampling and filter initialization
        bme.setTemperatureOversampling(BME680_OS_8X);
        bme.setHumidityOversampling(BME680_OS_2X);
        bme.setPressureOversampling(BME680_OS_4X);
        bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
        bme.setGasHeater(320, 150); // 320*C for 150 ms
    }
    SMB.println("-- Default Test --");

    SMB.println();
    return 1;
}

float get_temperature()
{
    return bme.readTemperature();
}

float get_pressure()
{
    return (bme.readPressure() / 100.0F);
}

float get_altitude(float seaLevelPressure)
{
    // return (1.0 - pow((float)get_pressure() / seaLevelPressure, 0.190284)) * 287.15 / 0.0065;

    return bme.readAltitude(seaLevelPressure);
}

float get_humidity()
{
    return bme.readHumidity();
}

float get_gas()
{
    return bme.readGas();
}