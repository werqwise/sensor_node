#include <Arduino.h>
#include "PMS.h"

// Serial pins
#define RX_PIN 39 // TX of Sensor
#define TX_PIN 15 // RX of Sensor

class PMSSensor
{
public:
    PMSSensor() : serial2(2), pms(serial2) {}

    int begin()
    {
        // Serial2.begin(9600);
        serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN); // Configure UART2 with specified pins

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
};
