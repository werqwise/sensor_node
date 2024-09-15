#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>

SensirionI2CScd4x scd4x;

// Struct to store last sensor values
struct SCD40SensorData
{
    uint16_t co2;
    float temperature;
    float humidity;
    bool isValid;
};

// Global variable to store last valid sensor reading
SCD40SensorData SensorData = {0, 0.0f, 0.0f, false};

void printUint16Hex(uint16_t value)
{
    SMB.print(value < 4096 ? "0" : "");
    SMB.print(value < 256 ? "0" : "");
    SMB.print(value < 16 ? "0" : "");
    SMB.print(value, HEX);
}

void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2)
{
    SMB.print("Serial: 0x");
    printUint16Hex(serial0);
    printUint16Hex(serial1);
    printUint16Hex(serial2);
    SMB.println();
}

int64_t setup_scd40()
{

    Wire.begin();

    uint16_t error;
    char errorMessage[256];

    scd4x.begin(Wire);

    // stop potentially previously started measurement
    error = scd4x.stopPeriodicMeasurement();
    if (error)
    {
        SMB.print("Error trying to execute stopPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        SMB.println(errorMessage);
        return 0;
    }

    uint16_t serial0;
    uint16_t serial1;
    uint16_t serial2;
    error = scd4x.getSerialNumber(serial0, serial1, serial2);
    if (error)
    {
        SMB.print("Error trying to execute getSerialNumber(): ");
        errorToString(error, errorMessage, 256);
        SMB.println(errorMessage);
    }
    else
    {
        printSerialNumber(serial0, serial1, serial2);
    }

    // Start Measurement
    error = scd4x.startPeriodicMeasurement();
    if (error)
    {
        SMB.print("Error trying to execute startPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        SMB.println(errorMessage);
    }

    SMB.println("Waiting for first measurement... (5 sec)");
    return 1;
}

// Getter function for sensor data
SCD40SensorData getSensorData()
{
    uint16_t error;
    char errorMessage[256];

    // Read Measurement
    uint16_t co2 = 0;
    float temperature = 0.0f;
    float humidity = 0.0f;
    bool isDataReady = false;

    error = scd4x.getDataReadyFlag(isDataReady);
    if (error)
    {
        SMB.print("Error trying to execute getDataReadyFlag(): ");
        errorToString(error, errorMessage, 256);
        SMB.println(errorMessage);
        return SensorData;
    }

    if (!isDataReady)
    {
        return SensorData;
    }

    error = scd4x.readMeasurement(co2, temperature, humidity);
    if (error)
    {
        SMB.print("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, 256);
        SMB.println(errorMessage);
        return SensorData;
    }
    else if (co2 == 0)
    {
        SMB.println("Invalid sample detected, skipping.");
        return SensorData;
    }
    else
    {
        // Update last valid sensor data
        SensorData.co2 = co2;
        SensorData.temperature = temperature;
        SensorData.humidity = humidity;
        SensorData.isValid = true;

        return SensorData;
    }
}

// Getter functions for individual sensor values
uint16_t getCO2()
{
    return getSensorData().co2;//in ppm
}

float getTemperature()
{
    return getSensorData().temperature;
}

float getHumidity()
{
    return getSensorData().humidity;
}