
// Sensor Node; Could be unlimited in number
#include <Arduino.h>
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
#include <PubSubClient.h>
#include "SStack.h"
#include <TaskScheduler.h> // Include TaskScheduler library
// Ethernet Configurations
#ifndef ETH_PHY_TYPE
#define ETH_PHY_TYPE  ETH_PHY_LAN8720
#define ETH_PHY_ADDR  0
#define ETH_PHY_MDC   23
#define ETH_PHY_MDIO  18
#define ETH_PHY_POWER -1
#define ETH_CLK_MODE  ETH_CLOCK_GPIO0_IN
#endif
static bool eth_connected = false;
#define LED 2 // GPIO number of connected LED, ON ESP-12 IS GPIO2

#define BLINK_PERIOD 3000  // milliseconds until cycle repeat
#define BLINK_DURATION 100 // milliseconds LED is on for

const char *device_type = "POE";
// MQTT Configurations
const char* mqtt_server = "mqtt.iot.werqwall.com";
const int mqtt_port = 1883;
const char* mqtt_user = "mqtt_client";
const char* mqtt_password = "mqtt_client";
String mqtt_topic;
// Prototypes
void sendMessage();
void getWeights();
void receivedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);

Scheduler userScheduler; // to control your personal task

bool calc_delay = false;

Task taskSendMessage(TASK_SECOND * 1.5, TASK_FOREVER, &sendMessage); // start with a one second interval
// WARNING: onEvent is called from a separate FreeRTOS task (thread)!
void onEvent(arduino_event_id_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      // The hostname must be set after the interface is started, but needs
      // to be set before DHCP, so set it from the event handler thread.
      ETH.setHostname("esp32-ethernet");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED: Serial.println("ETH Connected"); break;
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
    default: break;
  }
}
// Task to blink the number of nodes
Task blinkNoNodes;
bool onFlag = false;
String TrackerID = "";
float healthTimer = 0;
WiFiClient ethClient;
PubSubClient client(ethClient);
void setupMQTT() {
  client.setServer(mqtt_server, mqtt_port);
}

void sendMQTTMessage() {
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["type"] = device_type;  
  jsonDoc["time"] = millis();
  jsonDoc["temperature"] = get_temperature();
  jsonDoc["humidity"] = get_humidity();
  jsonDoc["pressure"] = get_pressure();
  jsonDoc["altitude"] = get_altitude();
  jsonDoc["noise_level"] = get_db();
  if (get_ens160_connection_status()) {
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

  client.publish(mqtt_topic.c_str(), buffer, 512);
}
void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
void setup()
{
  Serial.begin(115200);
  //Initialize Ethernet
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
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();
  WiFi.onEvent(onEvent);
  ETH.begin();
  randomSeed(analogRead(A0));
}

void loop()
{
  loop_limit_switch();

  digitalWrite(LED, !onFlag);
  //Send MQTT Message.
  while(!eth_connected){
    delay(100);
  }
  setupMQTT();
  connectMQTT();
}

void sendMessage()
{
  sendMQTTMessage();
  taskSendMessage.setInterval(TASK_SECOND * 1.5); // between 1 and 5 seconds
}
