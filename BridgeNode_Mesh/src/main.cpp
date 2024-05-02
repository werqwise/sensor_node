// This is a Bridge node send and receive data to/from Raspberry Pi over USB Serial connection.

#include <Arduino.h>
#include "SStack.h"
#define LED 2 // GPIO number of connected LED, ON ESP-12 IS GPIO2

#define BLINK_PERIOD 3000  // milliseconds until cycle repeat
#define BLINK_DURATION 100 // milliseconds LED is on for

#include "configs.h"
// Prototypes
void sendMessage();
void receivedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);
String serializeJsonToString(JsonObject obj);

Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

bool calc_delay = false;
SimpleList<uint32_t> nodes;

void sendMessage();                                                  // Prototype
Task taskSendMessage(TASK_SECOND * 0.5, TASK_FOREVER, &sendMessage); // start with a one second interval

// Task to blink the number of nodes
Task blinkNoNodes;
bool onFlag = false;
String TrackerID = "";

class BridgeManager
{
public:
  static BridgeManager *instance;
  // std::map<String, std::pair<JsonObject, int>> nodes; // MAC, (SensorData JSON object, RSSI)
  std::map<String, std::pair<String, int>> nodes; // MAC, (Serialized SensorData, RSSI)

  BridgeManager(painlessMesh &meshRef) : mesh(meshRef)
  {
  }
  void sendBroadcast(const String &message)
  {
    JsonDocument doc;
    doc["command"] = "broadcast";
    doc["data"] = message;
    String jsonMessage;
    serializeJson(doc, jsonMessage);
    mesh.sendBroadcast(jsonMessage);
  }

  void sendDataToUART()
  {
    JsonDocument doc;
    JsonArray nodeArray = doc.createNestedArray("nodes");
    if (nodes.empty())
    {
      Serial.println("{}");
    }
    else
    {
      for (auto &node : nodes)
      {
        JsonObject nodeObj = nodeArray.createNestedObject();
        nodeObj["macAddress"] = node.first;
        JsonDocument tempDoc;
        deserializeJson(tempDoc, node.second.first); // Deserialize sensor data from string
        nodeObj["data"] = tempDoc.as<JsonObject>();  // Set the data as JsonObject
        nodeObj["rssi"] = node.second.second;
      }
      String jsonMessage;
      serializeJson(doc, jsonMessage);
      Serial.println(jsonMessage);
    }
  }
  void printNodes()
  {
    JsonDocument doc;
    if (nodes.empty())
    {
      Serial.println("{}");
      return;
    }
    JsonArray array = doc.createNestedArray("nodes");

    doc["bridge_mac"] = TrackerID;
    for (auto &node : nodes)
    {
      JsonObject nodeObj = array.createNestedObject();
      nodeObj["macAddress"] = node.first;
      JsonDocument tempDoc;
      deserializeJson(tempDoc, node.second.first); // Deserialize from string
      nodeObj["data"] = tempDoc.as<JsonObject>();
      nodeObj["rssi"] = node.second.second;
    }

    // serializeJson(doc, Serial);
    String jsonMessage;
    serializeJson(doc, jsonMessage);
    Serial.println(jsonMessage);
  }

  int getNodeCount() const
  {
    return nodes.size();
  }

private:
  painlessMesh &mesh;
  JsonDocument doc; // Single large document to use for various operations
};

BridgeManager *BridgeManager::instance = nullptr;

void setup()
{
  Serial.begin(115200);
  TrackerID = String(WiFi.macAddress());

  // Serial.print("TrackerID: ");x
  // Serial.println(TrackerID);
  pinMode(LED, OUTPUT);
  mesh.setDebugMsgTypes(ERROR | DEBUG); // set before init() so that you can see error messages

  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(receivedCallback);
  mesh.onNewConnection(newConnectionCallback);
  mesh.onChangedConnections(changedConnectionCallback);
  mesh.onNodeTimeAdjusted(nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(delayReceivedCallback);
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
  BridgeManager::instance = new BridgeManager(mesh);
}

void loop()
{
  mesh.update();
  digitalWrite(LED, !onFlag);

  BridgeManager::instance->printNodes();
  // BridgeManager::instance->sendDataToUART();
  delay(2000);
}

void sendMessage()
{

  if (Serial.available())
  {
    String data = Serial.readStringUntil('\n');
    BridgeManager::instance->sendBroadcast(data);
  }
  taskSendMessage.setInterval(TASK_SECOND * 0.5); // Adjust as needed
}

void receivedCallback(uint32_t from, String &msg)
{
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, msg);
  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  String mac = doc["macAddress"].as<String>();
  int rssi = doc["rssi"];
  String data;
  serializeJson(doc["data"], data); // Serialize sensor data to string

  // Store serialized JSON and RSSI in the map
  BridgeManager::instance->nodes[mac] = std::make_pair(data, rssi);
}

String serializeJsonToString(JsonObject obj)
{
  String output;
  serializeJson(obj, output);
  return output;
}

void newConnectionCallback(uint32_t nodeId)
{
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);

  // Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
  // Serial.printf("--> startHere: New Connection, %s\n", mesh.subConnectionJson(true).c_str());
}

void changedConnectionCallback()
{
  // Serial.printf("Changed connections\n");
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000);

  nodes = mesh.getNodeList();
  // Serial.printf("nodeslist:::");
  // Serial.println(mesh.subConnectionJson());
  // Serial.println("nodes handler id list");

  // Serial.printf("Num nodes: %d\n", nodes.size());
  // Serial.printf("Node pointer: %d\n", nH.getNodesPointer());
  // Serial.printf("Connection list:");
  String NodeRemoved = "";
  SimpleList<uint32_t>::iterator node = nodes.begin();
  // nH.printAll();
  while (node != nodes.end())
  {
    // Serial.printf(" %u", *node);

    node++;
  }
  // Serial.println();
  calc_delay = true;
}

void nodeTimeAdjustedCallback(int32_t offset)
{
  // Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void delayReceivedCallback(uint32_t from, int32_t delay)
{
  // Serial.printf("Delay to node %u is %d us\n", from, delay);
}