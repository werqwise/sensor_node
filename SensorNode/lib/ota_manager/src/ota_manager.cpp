#include "ota_manager.h"

ESP32_OTA::ESP32_OTA(const char *serverUrl)
    : _serverUrl(serverUrl) {}

void ESP32_OTA::begin()
{
    _connectWiFi();
    _registerDevice();
}

void ESP32_OTA::handle()
{

    _checkForUpdates();
}

void ESP32_OTA::_connectWiFi()
{
    // Implement your Wi-Fi connection logic here
    uint8_t t=0;
    while (WiFi.status() != WL_CONNECTED)
    {
        t++;
        delay(500);
        Serial.print(".");
        if(t>=10){
            break;
        }
    }

    Serial.println("\nConnected to Wi-Fi.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void ESP32_OTA::_registerDevice()
{
    String macAddress = WiFi.macAddress();

    // Prepare JSON payload
    String payload = "{\"mac\": \"" + macAddress + "\"}";

    // Send HTTP POST request to server
    HTTPClient http;
    String url = String(_serverUrl) + "/register";

    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0)
    {
        String response = http.getString();
        Serial.println("Device registered successfully with the server.");
        Serial.println("Server response: " + response);
    }
    else
    {
        Serial.printf("Error registering device. HTTP response code: %d\n", httpResponseCode);
    }

    http.end();
}

void ESP32_OTA::_checkForUpdates()
{
    Serial.println("Checking for updates...");

    String macAddress = WiFi.macAddress();
    HTTPClient http;
    String url = String(_serverUrl) + "/firmware?mac=" + macAddress;

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK)
    {
        int contentLength = http.getSize();
        bool canBegin = Update.begin(contentLength);

        if (canBegin)
        {
            Serial.println("Begin OTA update");
            WiFiClient *client = http.getStreamPtr();
            size_t written = Update.writeStream(*client);

            if (written == contentLength)
            {
                Serial.println("Written : " + String(written) + " successfully");
            }
            else
            {
                Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?");
            }

            if (Update.end())
            {
                if (Update.isFinished())
                {
                    Serial.println("Update successfully completed. Rebooting.");
                    ESP.restart();
                }
                else
                {
                    Serial.println("Update not finished. Something went wrong.");
                }
            }
            else
            {
                Serial.println("Error Occurred. Error #: " + String(Update.getError()));
            }
        }
        else
        {
            Serial.println("Not enough space to begin OTA");
        }
    }
    else if (httpCode == HTTP_CODE_NO_CONTENT)
    {
        Serial.println("No updates available.");
    }
    else
    {
        Serial.printf("Failed to check for updates. HTTP response code: %d\n", httpCode);
    }

    http.end();
}