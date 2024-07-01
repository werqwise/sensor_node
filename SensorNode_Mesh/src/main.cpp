
// Sensor Node; Could be unlimited in number
#include <Arduino.h>
#include <driver/adc.h>
#include <ArduinoJson.h>
#include <SensorManager.h>
#include <bme280_handler.h>
#include <inmp441_handler.h>
#include <ens160_handler.h>
#include <mq135_handler.h>
#include <ldr_handler.h>
#include <limit_switch_handler.h>
#include <pir_handler.h>
#include <ETH.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "SStack.h"
#include <TaskScheduler.h> // Include TaskScheduler library

static bool eth_connected = false;
#define LED 2 // GPIO number of connected LED, ON ESP-12 IS GPIO2

#define BLINK_PERIOD 3000  // milliseconds until cycle repeat
#define BLINK_DURATION 100 // milliseconds LED is on for

const char *device_type = "POE";
// MQTT Configurations
const char *mqtt_server = "mqtt.iot.werqwall.com";
const int mqtt_port = 1883;
const char *mqtt_user = "mqtt_client";
const char *mqtt_password = "mqtt_client";
String mqtt_topic;
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

Scheduler userScheduler; // to control your personal task

bool calc_delay = false;

Task taskSendMessage(TASK_MINUTE * 5, TASK_FOREVER, &sendMessage); // start with a one second interval
// WARNING: onEvent is called from a separate FreeRTOS task (thread)!

void onEvent(arduino_event_id_t event)
{
  switch (event)
  {
  case ARDUINO_EVENT_ETH_START:
    Serial.println("ETH Started");
    // The hostname must be set after the interface is started, but needs
    // to be set before DHCP, so set it from the event handler thread.
    ETH.setHostname("esp32-ethernet");
    break;
  case ARDUINO_EVENT_ETH_CONNECTED:
    Serial.println("ETH Connected");
    break;
  case ARDUINO_EVENT_ETH_GOT_IP:
    Serial.println("ETH Got IP");
    Serial.println(ETH.localIP());
    eth_connected = true;
    break;
  case ARDUINO_EVENT_ETH_DISCONNECTED:
    Serial.println("ETH Disconnected");
    eth_connected = false;
    break;
  case ARDUINO_EVENT_ETH_STOP:
    Serial.println("ETH Stopped");
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
WiFiClient ethClient;
PubSubClient client(ethClient);

void setupMQTT()
{
  client.setServer(mqtt_server, mqtt_port);
}

void printMQTTMessage()
{
  JsonDocument jsonDoc;
  jsonDoc["type"] = device_type;
  jsonDoc["time"] = get_timestamp();
  jsonDoc["temperature"] = get_temperature();
  jsonDoc["humidity"] = get_humidity();
  jsonDoc["pressure"] = get_pressure();
  jsonDoc["altitude"] = get_altitude();
  jsonDoc["noise_level"] = get_db();
  if (get_ens160_connection_status())
  {
    jsonDoc["ens160_status"] = get_ens160_status();
    jsonDoc["aqi"] = get_aqi();
    jsonDoc["tvoc"] = get_tvoc();
    jsonDoc["eco2"] = get_eco2();
  }
  jsonDoc["mq135_aqi"] = get_aqi_mq135();
  jsonDoc["ldr"] = get_ldr();
  jsonDoc["limit_sw"] = get_limit_sw_state();
  jsonDoc["pir"] = get_pir();

  char buffer[512];
  serializeJson(jsonDoc, buffer);
  Serial.println("MQTT Payload");
  Serial.println(buffer);
  // client.publish(mqtt_topic.c_str(), buffer, 512);
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
  jsonDoc["altitude"] = get_altitude();
  jsonDoc["noise_level"] = get_db();
  if (get_ens160_connection_status())
  {
    jsonDoc["ens160_status"] = get_ens160_status();
    jsonDoc["aqi"] = String(get_aqi());
    jsonDoc["tvoc"] = String(get_tvoc());
    jsonDoc["eco2"] = String(get_eco2());
  }
  jsonDoc["mq135_aqi"] = get_aqi_mq135();
  jsonDoc["ldr"] = get_ldr();
  jsonDoc["limit_sw"] = get_limit_sw_state();
  jsonDoc["pir"] = get_pir();

  char buffer[512];

  size_t n = serializeJson(jsonDoc, buffer);
  client.publish(mqtt_topic.c_str(), buffer, n);
}
int connectMQTT()
{
  if (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_password))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
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
  Serial.begin(115200);
  // Wire.setPins(13, 16);
  // Wire.begin();
  // Initialize Ethernet
  TrackerID = String(WiFi.macAddress());
  Serial.print("TrackerID: ");
  Serial.println(TrackerID);
  mqtt_topic = "SensorData/" + TrackerID;
  Serial.print("MQTT Topic: ");
  Serial.println(mqtt_topic);

  pinMode(LED, OUTPUT);
  SensorManager sensors;
  sensors.auto_setup("BME680", setup_bme280, 5, 1);
  sensors.auto_setup("INMP441", setup_inmp441, 5, 1);
  sensors.auto_setup("ENS160", setup_ens160, 5, 1);
  sensors.auto_setup("LIMIT_SW", setup_limit_switch, 5, 1);
  sensors.auto_setup("PIR_SENSOR", setup_pir, 5, 1);
  if (get_ens160_connection_status())
  {
    calibrate_ens160(get_temperature(), get_humidity());
  }

  WiFi.onEvent(onEvent);
  ETH.begin();
  delay(100);

  // Initialize ADC
  analogReadResolution(12); // Optional: Set ADC resolution (default is 12 bits)

  // Use one of the ADC pins for the random seed
  randomSeed(micros());

  Serial.println("Random Seed initialized.");
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
  sendMQTTMessage();

  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();
}

void loop()
{
  loop_limit_switch();

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

  taskSendMessage.setInterval(TASK_MINUTE * 5); // Schedule next sending
}
