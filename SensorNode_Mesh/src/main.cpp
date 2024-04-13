
// Sensor Node; Could be unlimited in number
#include <Arduino.h>
#include "SStack.h"
#include <ArduinoJson.h>

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
    mesh.setDebugMsgTypes(ERROR | DEBUG);
    mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onChangedConnections(&changedConnectionCallback);
    mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
    mesh.onNodeDelayReceived(&delayReceivedCallback);
    mesh.setContainsRoot(true);
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
    for (const auto &kv : sensorData)
    {
      doc[kv.first] = kv.second;
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