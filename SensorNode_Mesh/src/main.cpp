
// Sensor Node; Could be unlimited in number
#include <Arduino.h>
#include "SStack.h"
#include <ArduinoJson.h>
#include <SensorManager.h>
#include <bme280_handler.h>
#include <inmp441_handler.h>
#include <ens160_handler.h>
#include <mq135_handler.h>
#include <ldr_handler.h>
#include <limit_switch_handler.h>
#include <pir_handler.h>

#define LED 2 // GPIO number of connected LED, ON ESP-12 IS GPIO2

#define BLINK_PERIOD 3000  // milliseconds until cycle repeat
#define BLINK_DURATION 100 // milliseconds LED is on for

#include "meshConfigs.h"
// Prototypes
void sendMessage();
void getWeights();
void receivedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);

Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

bool calc_delay = false;
SimpleList<uint32_t> nodes;

Task taskSendMessage(TASK_SECOND * 1.5, TASK_FOREVER, &sendMessage); // start with a one second interval

// Task to blink the number of nodes
Task blinkNoNodes;
bool onFlag = false;
String TrackerID = "";
float healthTimer = 0;

class MeshManager
{
public:
  MeshManager(painlessMesh &meshRef) : mesh(meshRef)
  {
  }

  void addSensor(const String &name, const String &value)
  {
    sensorData[name] = value;
  }

  void send()
  {

    JsonDocument doc; // Adjust size as necessary based on expected JSON size

    doc["macAddress"] = WiFi.macAddress();
    doc["rssi"] = WiFi.RSSI();
    JsonObject dataObj = doc.createNestedObject("data");
    for (const auto &kv : sensorData)
    {
      dataObj[kv.first] = kv.second;
    }

    String msg;
    serializeJson(doc, msg);
    mesh.sendBroadcast(msg);
    Serial.println("Sending JSON: " + msg);
  }

private:
  painlessMesh &mesh;
  std::map<String, String> sensorData;
};

MeshManager meshManager(mesh);

void setup()
{
  Serial.begin(115200);

  TrackerID = String(WiFi.macAddress());
  Serial.print("TrackerID: ");
  Serial.println(TrackerID);
  pinMode(LED, OUTPUT);
  SensorManager sensors;
  sensors.auto_setup("BME680", setup_bme280, 5, 1);
  sensors.auto_setup("INMP441", setup_inmp441, 5, 1);
  sensors.auto_setup("ENS160", setup_ens160, 5, 1);
  sensors.auto_setup("LIMIT_SW", setup_limit_switch, 5, 1);
  // sensors.auto_setup("PIR_SENSOR", setup_pir, 5, 1);
  if (get_ens160_connection_status())
  {
    calibrate_ens160(get_temperature(), get_humidity());
  }
  mesh.setDebugMsgTypes(ERROR | DEBUG);
  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);
  mesh.setContainsRoot(true);

  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();

  blinkNoNodes.set(BLINK_PERIOD, (mesh.getNodeList().size() + 1) * 2, []()
                   {
                     // If on, switch off, else switch on
                     if (onFlag)
                       onFlag = false;
                     else
                       onFlag = true;
                     blinkNoNodes.delay(BLINK_DURATION);

                     if (blinkNoNodes.isLastIteration())
                     {
                       // Finished blinking. Reset task for next run
                       // blink number of nodes (including this node) times
                       blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
                       // Calculate delay based on current mesh time and BLINK_PERIOD
                       // This results in blinks between nodes being synced
                       blinkNoNodes.enableDelayed(BLINK_PERIOD -
                                                  (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);
                     } });
  userScheduler.addTask(blinkNoNodes);
  blinkNoNodes.enable();

  randomSeed(analogRead(A0));
}

void loop()
{
  mesh.update();
  loop_limit_switch();

  digitalWrite(LED, !onFlag);
}

void sendMessage()
{

  if (calc_delay)
  {
    SimpleList<uint32_t>::iterator node = nodes.begin();
    while (node != nodes.end())
    {
      mesh.startDelayMeas(*node);
      node++;
    }
    calc_delay = false;
  }
  meshManager.addSensor("time", String(millis()));
  meshManager.addSensor("temperature", String(get_temperature()));
  meshManager.addSensor("humidity", String(get_humidity()));
  meshManager.addSensor("pressure", String(get_pressure()));
  meshManager.addSensor("altitude", String(get_altitude()));
  meshManager.addSensor("noise_level", String(get_db()));
  if (get_ens160_connection_status())
  {
    meshManager.addSensor("ens160_status", String(get_ens160_status()));
    meshManager.addSensor("aqi", String(get_aqi()));
    meshManager.addSensor("tvoc", String(get_tvoc()));
    meshManager.addSensor("eco2", String(get_eco2()));
  }
  meshManager.addSensor("mq135_aqi", String(get_aqi_mq135()));
  meshManager.addSensor("ldr", String(get_ldr()));
  meshManager.addSensor("limit_sw", String(get_limit_sw_state()));
  meshManager.addSensor("pir", String(get_pir()));

  meshManager.send();
  taskSendMessage.setInterval(TASK_SECOND * 1.5); // between 1 and 5 seconds
}

void receivedCallback(uint32_t from, String &msg)
{
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  JsonDocument doc;
  deserializeJson(doc, msg);
  String macAddress = doc["macAddress"];
  if (macAddress == WiFi.macAddress())
  {
    Serial.println("Received message.");
  }
  else if (macAddress == String("*"))
  {
    Serial.println("Received message; broadcasted to all nodes.");
  }
}

void newConnectionCallback(uint32_t nodeId)
{
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);

  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
  Serial.printf("--> startHere: New Connection, %s\n", mesh.subConnectionJson(true).c_str());
}

void changedConnectionCallback()
{
  Serial.printf("Changed connections\n");
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);

  nodes = mesh.getNodeList();

  Serial.printf("Num nodes: %d\n", nodes.size());
  Serial.printf("Connection list:");

  SimpleList<uint32_t>::iterator node = nodes.begin();
  while (node != nodes.end())
  {
    Serial.printf(" %u", *node);
    node++;
  }
  Serial.println();
  calc_delay = true;
}

void nodeTimeAdjustedCallback(int32_t offset)
{
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void delayReceivedCallback(uint32_t from, int32_t delay)
{
  Serial.printf("Delay to node %u is %d us\n", from, delay);
}