
// Sensor Node; Could be unlimited in number
#include <Arduino.h>
#include <transparent_serial.h>
#include <driver/adc.h>
#include <ArduinoJson.h>
#include <ETH.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "SStack.h"
#include <TaskScheduler.h> // Include TaskScheduler library
#include <ota_manager.h>

WiFiClient ethClient;
PubSubClient client(ethClient);

// Create an instance of MySerial
// Using String for topic
String mqttTopic = String("SensorData/") + String(WiFi.macAddress()) + String("/logs");
MqttSerial SMB(Serial, client, mqttTopic);

// Server URL (replace with your server's address)
const char *serverUrl = "http://ota-app.iot.werqwall.com";

ESP32_OTA ota(serverUrl);

#include <SensorManager.h>
#include <bme680_handler.h>
#include <inmp441_handler.h>
#include <pms_handler.h>
#include <scd40_handler.h>
#include <ldr_handler.h>
#include <ld2410_handler.h>
#include <limit_switch_handler.h>
#include <pir_handler.h>
#include <esp32_comm.h>
#include <HTTPClient.h>

static bool eth_connected = false;
#define LED 2 // GPIO number of connected LED, ON ESP-12 IS GPIO2

#define BLINK_PERIOD 3000  // milliseconds until cycle repeat
#define BLINK_DURATION 100 // milliseconds LED is on for

const char *device_type = "POE";
// API servers
const char *ipServer = "https://api.ipify.org?format=json";
const char *geoServer = "http://ip-api.com";
const char *weatherServer = "https://api.open-meteo.com";
// MQTT Configurations
const char *mqtt_server = "mqtt.iot.werqwall.com";
const int mqtt_port = 1883;
const char *mqtt_user = "mqtt_client";
const char *mqtt_password = "mqtt_client";
String mqtt_topic;
String uwb_msg = "";
uint8_t do_ota = 0;
// Prototypes
void sendMessage();
int connectMQTT();
void getWeights();
void printMQTTMessage();
void receivedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);
void customLongPressStopFunction(void *oneButton);
void onESP32WiFiReceive(String message);
void mqtt_callback(char *topic, byte *payload, unsigned int length);
float getSeaLevelPressure(float latitude, float longitude);
String getPublicIP();
Scheduler userScheduler; // to control your personal task

bool calc_delay = false;

Task taskSendMessage(TASK_SECOND * 5, TASK_FOREVER, &sendMessage); // start with a one second interval
// WARNING: onEvent is called from a separate FreeRTOS task (thread)!

PMSSensor pms_sensor;
SensorManager sensors;

ESP32Comm comm(true, "master-esp32", "master-pass"); // Master example

void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
  // Convert topic to Arduino String
  String topicStr = String(topic);

  // Convert payload to Arduino String
  String payloadStr;
  for (int i = 0; i < length; i++)
  {
    payloadStr += (char)payload[i];
  }

  // Print the topic and payload for debugging
  String ota_topic = String("SensorData/") + String(WiFi.macAddress()) + String("/do_ota");
  SMB.print("Message arrived [");
  SMB.print(topicStr);
  SMB.print("] ");
  SMB.println(payloadStr);

  // Check if the topic is "macaddress/do_ota" and payload contains "do_ota"
  if (topicStr.equals(ota_topic) && payloadStr.indexOf("do_ota") >= 0)
  {
    ota.begin();
    ota.handle();
  }
}

void onEvent(arduino_event_id_t event)
{
  switch (event)
  {
  case ARDUINO_EVENT_ETH_START:
    SMB.println("ETH Started");
    // The hostname must be set after the interface is started, but needs
    // to be set before DHCP, so set it from the event handler thread.
    ETH.setHostname("esp32-ethernet");
    break;
  case ARDUINO_EVENT_ETH_CONNECTED:
    SMB.println("ETH Connected");
    break;
  case ARDUINO_EVENT_ETH_GOT_IP:
    SMB.println("ETH Got IP");
    SMB.println(ETH.localIP());
    eth_connected = true;
    break;
  case ARDUINO_EVENT_ETH_DISCONNECTED:
    SMB.println("ETH Disconnected");
    eth_connected = false;
    break;
  case ARDUINO_EVENT_ETH_STOP:
    SMB.println("ETH Stopped");
    eth_connected = false;
    break;
  default:
    break;
  }
}
// Task to blink the number of nodes
Task blinkNoNodes;
bool onFlag = false;
String TrackerID = "";
float healthTimer = 0;

void setupMQTT()
{
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback);
  client.setBufferSize(512);
}

String lastPublishedUWBMessage = "";

void onESP32WiFiReceive(String message)
{
  SMB.println("Received from slave: " + message);
  uwb_msg = message;

  // Check if the new message is different from the last published one
  if (message != lastPublishedUWBMessage)
  {
    String topic = mqtt_topic + String("/uwb");
    client.publish(topic.c_str(), message.c_str());
    lastPublishedUWBMessage = message; // Update the last published message
    SMB.println("Published new message: " + message);
  }
  else
  {
    SMB.println("Message unchanged, not publishing");
  }
}

// Function to get public IP address
String getPublicIP()
{
  if (eth_connected)
  {
    HTTPClient http;
    String publicIP;
    String status = "";
    http.begin("https://api.ipify.org"); // Specify the URL

    status = http.GET() > 0 ? http.getString() : "N\\A";

    http.end(); // Free the resources
    return status;
  }
}
// Function to get geolocation based on public IP
bool getGeoLocation(String publicIP, float &latitude, float &longitude)
{
  if (eth_connected)
  {
    HTTPClient http;
    String url = "http://ip-api.com/json/" + publicIP;

    http.begin(url); // Specify the URL

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0)
    {
      String payload = http.getString();

      // Parse JSON response
      const size_t capacity = JSON_OBJECT_SIZE(14) + 350;
      DynamicJsonDocument doc(capacity);
      DeserializationError error = deserializeJson(doc, payload);
      if (error)
      {
        SMB.print("Failed to parse geolocation JSON: ");
        SMB.println(error.c_str());
        http.end();
        return false;
      }

      String status = doc["status"].as<String>();
      if (status != "success")
      {
        SMB.println("Geolocation API returned an error");
        http.end();
        return false;
      }

      latitude = doc["lat"].as<float>();
      longitude = doc["lon"].as<float>();

      http.end();
      return true;
    }
    else
    {
      SMB.print("Error on HTTP request: ");
      SMB.println(httpResponseCode);
      http.end();
      return false;
    }
  }
  else
  {
    SMB.println("Ethernet not connected");
    return false;
  }
}

float getSeaLevelPressure(float latitude, float longitude)
{
  if (eth_connected)
  {
    HTTPClient http;
    String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(latitude, 6) +
                 "&longitude=" + String(longitude, 6) + "&hourly=surface_pressure";

    http.begin(url);

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0)
    {
      String payload = http.getString();

      // Parse JSON response
      DynamicJsonDocument doc(8192); // Adjust size as needed

      DeserializationError error = deserializeJson(doc, payload);
      if (error)
      {
        SMB.print("JSON deserialization failed: ");
        SMB.println(error.c_str());
        http.end();
        return 1013.25; // Default sea-level pressure
      }

      JsonArray timeArray = doc["hourly"]["time"];
      JsonArray pressureArray = doc["hourly"]["surface_pressure"];

      if (timeArray.size() > 0 && pressureArray.size() > 0)
      {
        // Get the last available data point (latest time and pressure)
        int lastIndex = timeArray.size() - 1;
        float seaLevelPressure = pressureArray[lastIndex].as<float>();

        // SMB.print("Latest available time: ");
        // SMB.println(timeArray[lastIndex].as<const char *>()); // Log the latest time for debugging

        http.end();
        return seaLevelPressure;
      }
      else
      {
        SMB.println("No hourly data found");
        http.end();
        return 1018.7; // Default sea-level pressure
      }
    }
    else
    {
      SMB.print("Error on HTTP request: ");
      SMB.println(httpResponseCode);
      http.end();
      return 1018.7; // Default sea-level pressure
    }
  }
  else
  {
    SMB.println("Ethernet not connected");
    return 1018.7;
  }
}
void printMQTTMessage()
{
  JsonDocument jsonDoc;
  jsonDoc["type"] = device_type;
  jsonDoc["time"] = get_timestamp();
  jsonDoc["temperature"] = get_temperature();
  jsonDoc["humidity"] = get_humidity();
  jsonDoc["pressure"] = get_pressure();
  // jsonDoc["altitude"] = get_altitude();
  jsonDoc["gas"] = get_gas();
  jsonDoc["noise_level"] = get_db();
  jsonDoc["PM_AE_UG_1_0"] = pms_sensor.getPM1_0();
  jsonDoc["PM_AE_UG_2_5"] = pms_sensor.getPM2_5();
  jsonDoc["PM_AE_UG_10_0"] = pms_sensor.getPM10_0();
  jsonDoc["co2"] = getCO2();
  jsonDoc["ldr"] = get_ldr();
  if (radarInterface.isTargetPresent())
  {
    jsonDoc["radar_stationary_dist"] = radarInterface.getStationaryDistance();
    jsonDoc["radar_stationary_energy"] = radarInterface.getStationaryEnergy();
    jsonDoc["radar_moving_dist"] = radarInterface.getMovingDistance();
    jsonDoc["radar_moving_energy"] = radarInterface.getMovingEnergy();
  }
  jsonDoc["limit_sw"] = get_limit_sw_state();
  jsonDoc["pir"] = get_pir();

  char buffer[512];
  serializeJson(jsonDoc, buffer);
  SMB.println("MQTT Payload");
  SMB.println(buffer);
  // client.publish(mqtt_topic.c_str(), buffer, 512);
}

int esp32_wifi_comm_begin()
{
  comm.begin();
  comm.setDataCallback(onESP32WiFiReceive);

  return 1;
}

void sendMQTTMessage()
{

  JsonDocument jsonDoc;
  jsonDoc["type"] = device_type;
  jsonDoc["bridge_mac"] = TrackerID;
  jsonDoc["time"] = get_timestamp();
  jsonDoc["temperature"] = get_temperature();
  jsonDoc["humidity"] = get_humidity();
  jsonDoc["pressure"] = get_pressure();
  // Get geolocation
  float latitude = 0.0, longitude = 0.0;
  if (!getGeoLocation(getPublicIP(), latitude, longitude))
  {
    SMB.println("Unable to obtain geolocation");
  }
  else
  {
    float seaLevelPressure = getSeaLevelPressure(latitude, longitude);
    jsonDoc["altitude"] = get_altitude(seaLevelPressure);
    jsonDoc["latitude"] = String(latitude);
    jsonDoc["longitude"] = String(longitude);
    jsonDoc["seaLevelPressures"] = String(seaLevelPressure);
  }

  jsonDoc["gas"] = get_gas();
  jsonDoc["noise_level"] = get_db();
  jsonDoc["PM_AE_UG_1_0"] = pms_sensor.getPM1_0();
  jsonDoc["PM_AE_UG_2_5"] = pms_sensor.getPM2_5();
  jsonDoc["PM_AE_UG_10_0"] = pms_sensor.getPM10_0();
  jsonDoc["co2"] = getCO2();
  if (radarInterface.isTargetPresent())
  {
    jsonDoc["radar_stationary_dist"] = radarInterface.getStationaryDistance();
    jsonDoc["radar_stationary_energy"] = radarInterface.getStationaryEnergy();
    jsonDoc["radar_moving_dist"] = radarInterface.getMovingDistance();
    jsonDoc["radar_moving_energy"] = radarInterface.getMovingEnergy();
  }
  jsonDoc["ldr"] = get_ldr();
  jsonDoc["limit_sw"] = get_limit_sw_state();
  jsonDoc["pir"] = get_pir();
  jsonDoc["publicIp"] = getPublicIP();
  jsonDoc["version"] = String("v1.1");
  char buffer[512];

  size_t n = serializeJson(jsonDoc, buffer);
  client.publish(mqtt_topic.c_str(), buffer, n);
  comm.send("POE;Hello from Master!");

  uwb_msg = "";
}
int connectMQTT()
{
  if (!client.connected())
  {
    SMB.print("Attempting MQTT connection...");
    String clientID = randomString(TrackerID);
    if (client.connect(clientID.c_str(), mqtt_user, mqtt_password))
    {
      SMB.println("connected");
    }
    else
    {
      SMB.print("failed, rc=");
      SMB.print(client.state());
      SMB.println(" try again in 5 seconds");
      delay(5000);
    }
    return 0;
  }
  else
  {
    return 1;
  }
}
void setup()
{
  SMB.begin(115200);
  // Wire.setPins(13, 16);
  // Wire.begin();
  // Initialize Ethernet
  TrackerID = String(WiFi.macAddress());
  SMB.print("TrackerID: ");
  SMB.println(TrackerID);
  mqtt_topic = "SensorData/" + TrackerID;
  SMB.print("MQTT Topic: ");
  SMB.println(mqtt_topic);

  pinMode(LED, OUTPUT);
  sensors.auto_setup("ESP32_WIFI_COMM", esp32_wifi_comm_begin, 5, 1);
  sensors.auto_setup("BME680", setup_bme680, 5, 1);
  sensors.auto_setup("INMP441", setup_inmp441, 5, 1);
  sensors.auto_setup("LIMIT_SW", setup_limit_switch, 5, 1);
  sensors.auto_setup("PIR_SENSOR", setup_pir, 5, 1);
  sensors.auto_setup("SCD40_SENSOR", setup_scd40, 5, 1);
  sensors.auto_setup("LD2410_SENSOR", setup_ld2410, 5, 1);

  pms_sensor.begin();

  setLongPressStopCallback(customLongPressStopFunction);

  WiFi.onEvent(onEvent);
  ETH.begin();
  delay(100);

  // Initialize ADC
  analogReadResolution(12); // Optional: Set ADC resolution (default is 12 bits)

  // Use one of the ADC pins for the random seed
  randomSeed(micros());

  SMB.println("Random Seed initialized.");
  // for (int i = 0; i < 20; i++)
  // {
  //   printMQTTMessage();
  // }
  client.setBufferSize(512);
  setupMQTT();
  while (!connectMQTT())
  {
    delay(2000);
  }

  delay(100);
  String ota_topic_subs = String("SensorData/") + String(WiFi.macAddress()) + String("/do_ota");
  client.subscribe(ota_topic_subs.c_str());
  sendMQTTMessage();

  userScheduler.addTask(taskSendMessage);

  taskSendMessage.enable();
}
void customLongPressStopFunction(void *oneButton)
{
  if (is_limit_sw_state_changed())
  {
    sendMQTTMessage();
  }
  clear_limit_sw_state();
}

void loop()
{
  SMB.loop();

  loop_limit_switch();
  comm.loop(); // This ensures the communication module processes any incoming messages

  pms_sensor.pms_loop();
  if (eth_connected && connectMQTT())
  {
    digitalWrite(LED, false);
  }
  else
  {
    digitalWrite(LED, true);
  }
  // Send MQTT Message.
  while (!eth_connected)
  {

    delay(100);
    userScheduler.execute(); // Run the scheduler
  }

  client.loop(); // Ensure the MQTT client is looped

  userScheduler.execute(); // Run the scheduler
}

void sendMessage()
{

  sendMQTTMessage();

  taskSendMessage.setInterval(TASK_SECOND * 5); // Schedule next sending
}
