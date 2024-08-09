
// Sensor Node; Could be unlimited in number
#include <Arduino.h>
#include <driver/adc.h>
#include <ArduinoJson.h>
#include <SensorManager.h>
#include <bme680_handler.h>
#include <inmp441_handler.h>
#include <pms_handler.h>
#include <scd40_handler.h>
#include <ldr_handler.h>
#include <ld2410_handler.h>
#include <limit_switch_handler.h>
#include <pir_handler.h>
#include <ETH.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "SStack.h"
#include <TaskScheduler.h> // Include TaskScheduler library
#include <esp32_comm.h>

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
String uwb_msg = "";
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

Scheduler userScheduler; // to control your personal task

bool calc_delay = false;

Task taskSendMessage(TASK_SECOND * 5, TASK_FOREVER, &sendMessage); // start with a one second interval
// WARNING: onEvent is called from a separate FreeRTOS task (thread)!

PMSSensor pms_sensor;
SensorManager sensors;

ESP32Comm comm(true, "master-esp32", "master-pass"); // Master example

void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

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
  client.setCallback(mqtt_callback);
  client.setBufferSize(512);
}

String lastPublishedUWBMessage = "";

void onESP32WiFiReceive(String message)
{
  Serial.println("Received from slave: " + message);
  uwb_msg = message;

  // Check if the new message is different from the last published one
  if (message != lastPublishedUWBMessage)
  {
    String topic = mqtt_topic + String("/uwb");
    client.publish(topic.c_str(), message.c_str());
    lastPublishedUWBMessage = message; // Update the last published message
    Serial.println("Published new message: " + message);
  }
  else
  {
    Serial.println("Message unchanged, not publishing");
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
  jsonDoc["altitude"] = get_altitude();
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
  Serial.println("MQTT Payload");
  Serial.println(buffer);
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
  jsonDoc["altitude"] = get_altitude();
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
    Serial.print("Attempting MQTT connection...");
    String clientID = randomString(TrackerID);
    if (client.connect(clientID.c_str(), mqtt_user, mqtt_password))
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
