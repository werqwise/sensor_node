/*
 * Example sketch for reporting on readings from the LD2410 using whatever settings are currently configured.
 *
 * This has been tested on the following platforms...
 *
 * On ESP32, connect the LD2410 to GPIO pins 32&33
 * On ESP32S2, connect the LD2410 to GPIO pins 8&9
 * On ESP32C3, connect the LD2410 to GPIO pins 4&5
 * On Arduino Leonardo or other ATmega32u4 board connect the LD2410 to GPIO pins TX & RX hardware serial
 *
 * The serial configuration for other boards will vary and you'll need to assign them yourself
 *
 * There is no example for ESP8266 as it only has one usable UART and will not boot if the alternate UART pins are used for the radar.
 *
 * For this sketch and other examples to be useful the board needs to have two usable UARTs.
 *
 */

#if defined(ESP32)

#define MONITOR_SERIAL Serial
#define RADAR_SERIAL Serial1
#define RADAR_RX_PIN 4
#define RADAR_TX_PIN 5
#endif

#include <ld2410.h>

ld2410 radar;

uint32_t lastReading = 0;
bool radarConnected = false;

int setup_ld2410()
{

    // radar.debug(MONITOR_SERIAL); //Uncomment to show debug information from the library on the Serial Monitor. By default this does not show sensor reads as they are very frequent.

    RADAR_SERIAL.begin(256000, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN); // UART for monitoring the radar

    delay(500);
    MONITOR_SERIAL.print(F("\nConnect LD2410 radar TX to GPIO:"));
    MONITOR_SERIAL.println(RADAR_RX_PIN);
    MONITOR_SERIAL.print(F("Connect LD2410 radar RX to GPIO:"));
    MONITOR_SERIAL.println(RADAR_TX_PIN);
    MONITOR_SERIAL.print(F("LD2410 radar sensor initialising: "));
    if (radar.begin(RADAR_SERIAL))
    {
        MONITOR_SERIAL.println(F("OK"));
        MONITOR_SERIAL.print(F("LD2410 firmware version: "));
        MONITOR_SERIAL.print(radar.firmware_major_version);
        MONITOR_SERIAL.print('.');
        MONITOR_SERIAL.print(radar.firmware_minor_version);
        MONITOR_SERIAL.print('.');
        MONITOR_SERIAL.println(radar.firmware_bugfix_version, HEX);
        return 1;
    }
    else
    {
        MONITOR_SERIAL.println(F("not connected"));
        return 0;
    }
}

struct RadarData {
    int stationaryDistance;
    int stationaryEnergy;
    int movingDistance;
    int movingEnergy;
    bool isPresent;
    unsigned long lastUpdateTime;
};
class RadarInterface {
private:
    RadarData lastData;
    const unsigned long updateInterval = 1000; // 1 second interval

    void updateData() {
        unsigned long currentTime = millis();
        if (currentTime - lastData.lastUpdateTime >= updateInterval) {
            radar.read();
            if (radar.isConnected()) {
                lastData.isPresent = radar.presenceDetected();
                if (lastData.isPresent) {
                    if (radar.stationaryTargetDetected()) {
                        lastData.stationaryDistance = radar.stationaryTargetDistance();
                        lastData.stationaryEnergy = radar.stationaryTargetEnergy();
                    }
                    if (radar.movingTargetDetected()) {
                        lastData.movingDistance = radar.movingTargetDistance();
                        lastData.movingEnergy = radar.movingTargetEnergy();
                    }
                }
                lastData.lastUpdateTime = currentTime;
            }
        }
    }

public:
    RadarInterface() {
        lastData = {0, 0, 0, 0, false, 0};
    }

    int getStationaryDistance() {
        updateData();
        return lastData.stationaryDistance;
    }

    int getStationaryEnergy() {
        updateData();
        return lastData.stationaryEnergy;
    }

    int getMovingDistance() {
        updateData();
        return lastData.movingDistance;
    }

    int getMovingEnergy() {
        updateData();
        return lastData.movingEnergy;
    }

    bool isTargetPresent() {
        updateData();
        return lastData.isPresent;
    }
};

// Global instance of RadarInterface

// Global instance of RadarInterface
RadarInterface radarInterface;
