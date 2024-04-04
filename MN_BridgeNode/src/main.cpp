//This is a Bridge node send and receive data to/from Raspberry Pi over USB Serial connection.

#include <Arduino.h>
#include "SStack.h"
#include "nodesHandler.h"

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

Scheduler userScheduler; // to control your personal task
painlessMesh mesh;
NodesHandler nH;

bool calc_delay = false;
SimpleList<uint32_t> nodes;

void sendMessage();                                                  // Prototype
Task taskSendMessage(TASK_SECOND * 0.5, TASK_FOREVER, &sendMessage); // start with a one second interval

// Task to blink the number of nodes
Task blinkNoNodes;
bool onFlag = false;
String TrackerID = "";

void setup()
{
  Serial.begin(115200); //Don't comment it; else the serial communication over USB will not work.
  TrackerID = String(WiFi.macAddress());
  TrackerID = StringSeparator(TrackerID, ':', 0) + StringSeparator(TrackerID, ':', 1) + StringSeparator(TrackerID, ':', 2) + StringSeparator(TrackerID, ':', 3) +
              StringSeparator(TrackerID, ':', 4) + StringSeparator(TrackerID, ':', 4) + StringSeparator(TrackerID, ':', 5);
  // Serial.print("TrackerID: ");
  // Serial.println(TrackerID);
  pinMode(LED, OUTPUT);

  mesh.setDebugMsgTypes(ERROR | DEBUG); // set before init() so that you can see error messages

  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
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
                     }
                   });
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

  nH.sendDataToUART(mesh.subConnectionJson());
  if (Serial.available())
  {
    String dataRec = Serial.readString();
    if (dataRec.indexOf("B:") >= 0)
    { //broadcast only if the incomming data contains B: keyword
      String ack = StringSeparator(dataRec, ':', 1);

      mesh.sendBroadcast(ack); //broadcast to all nodes
      ack = String("ACK: ") + ack;
      Serial.println(ack); //send ack to python script
    }
  }
  taskSendMessage.setInterval(TASK_SECOND * 0.5); // send data after every 1 second
}

void receivedCallback(uint32_t from, String &msg)
{

  String NodeMAC = StringSeparator(msg, ';', 0);
  String NodeID = StringSeparator(msg, ';', 1);
  String NodeData = StringSeparator(msg, ';', 2);
  // Serial.println("msg arrived::");
  // Serial.println(msg);

  nH.addNode(NodeMAC, NodeData, NodeID);

  //nH.printAll();
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
  Serial.printf("Changed connections\n");
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
    Serial.printf(" %u", *node);

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