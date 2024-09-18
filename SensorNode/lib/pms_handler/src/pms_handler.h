#include <Arduino.h>
#include "PMS.h"

// Serial pins
#define S2_RX_PIN 39 // TX of Sensor
#define S2_TX_PIN 15 // RX of Sensor

class PMSSensor
{
public:
    PMSSensor() : serial2(2), pms(serial2) {}

    int begin()
    {
        // Serial2.begin(9600);
        serial2.begin(9600, SERIAL_8N1, S2_RX_PIN, S2_TX_PIN); // Configure UART2 with specified pins

        pms.passiveMode();
        pms.sleep();
        lastReadTime = millis() - READ_INTERVAL + FIRST_READ_DELAY; // Schedule first read after 3 seconds
        return 1;
    }

    void pms_loop()
    {
        unsigned long currentTime = millis();

        switch (currentState)
        {
        case SLEEPING:
            if (currentTime - lastReadTime >= READ_INTERVAL)
            {
                pms.wakeUp();
                currentState = WAKING_UP;
                stateStartTime = currentTime;
            }
            break;

        case WAKING_UP:
            if (currentTime - stateStartTime >= 30000)
            { // 30 seconds warm-up
                currentState = READING;
                pms.requestRead();
            }
            break;

        case READING:
            PMS::DATA data;
            if (pms.readUntil(data, 1000))
            {
                pmsData.PM_AE_UG_1_0 = data.PM_AE_UG_1_0;
                pmsData.PM_AE_UG_2_5 = data.PM_AE_UG_2_5;
                pmsData.PM_AE_UG_10_0 = data.PM_AE_UG_10_0;
                pmsData.dataReady = true;

                pms.sleep();
                currentState = SLEEPING;
                lastReadTime = currentTime;
            }
            else if (currentTime - stateStartTime >= 5000)
            { // Timeout after 5 seconds
                pms.sleep();
                currentState = SLEEPING;
                lastReadTime = currentTime;
            }
            break;
        }
    }

    uint16_t getPM1_0() { return pmsData.PM_AE_UG_1_0; }
    uint16_t getPM2_5() { return pmsData.PM_AE_UG_2_5; }
    uint16_t getPM10_0() { return pmsData.PM_AE_UG_10_0; }
    bool isDataReady() { return pmsData.dataReady; }
    void resetDataReady() { pmsData.dataReady = false; }

    // Function to calculate AQI based on PM2.5 and PM10
    int calculateAQI()
    {
        int aqi_pm25 = calculatePM25AQI(pmsData.PM_AE_UG_2_5);
        int aqi_pm10 = calculatePM10AQI(pmsData.PM_AE_UG_10_0);

        // Return the higher AQI of the two (PM2.5 or PM10)
        return max(aqi_pm25, aqi_pm10);
    }

private:
    HardwareSerial serial2; // Use HardwareSerial instead of SoftwareSerial

    PMS pms;

    // Global struct to store sensor data
    struct PMSData
    {
        uint16_t PM_AE_UG_1_0 = 0;
        uint16_t PM_AE_UG_2_5 = 0;
        uint16_t PM_AE_UG_10_0 = 0;
        bool dataReady = false;
    } pmsData;

    // Timing variables
    unsigned long lastReadTime = 0;
    const unsigned long READ_INTERVAL = 20000;   // 20 seconds
    const unsigned long FIRST_READ_DELAY = 3000; // 3 seconds

    enum PMSState
    {
        SLEEPING,
        WAKING_UP,
        READING
    };

    PMSState currentState = SLEEPING;
    unsigned long stateStartTime = 0;
    // Helper functions to calculate AQI for PM2.5 and PM10
    int calculatePM25AQI(uint16_t pm25)
    {
        if (pm25 <= 12.0)
            return map(pm25, 0.0, 12.0, 0, 50);
        else if (pm25 <= 35.4)
            return map(pm25, 12.1, 35.4, 51, 100);
        else if (pm25 <= 55.4)
            return map(pm25, 35.5, 55.4, 101, 150);
        else if (pm25 <= 150.4)
            return map(pm25, 55.5, 150.4, 151, 200);
        else if (pm25 <= 250.4)
            return map(pm25, 150.5, 250.4, 201, 300);
        else if (pm25 <= 500.4)
            return map(pm25, 250.5, 500.4, 301, 500);
        return -1; // Invalid range
    }

    int calculatePM10AQI(uint16_t pm10)
    {
        if (pm10 <= 54.0)
            return map(pm10, 0.0, 54.0, 0, 50);
        else if (pm10 <= 154.0)
            return map(pm10, 55, 154, 51, 100);
        else if (pm10 <= 254.0)
            return map(pm10, 155, 254, 101, 150);
        else if (pm10 <= 354.0)
            return map(pm10, 255, 354, 151, 200);
        else if (pm10 <= 424.0)
            return map(pm10, 355, 424, 201, 300);
        else if (pm10 <= 604.0)
            return map(pm10, 425, 604, 301, 500);
        return -1; // Invalid range
    }
};
