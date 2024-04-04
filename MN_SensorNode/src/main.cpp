
//Sensor Node; Could be unlimited in number
#include <Arduino.h>
#include "SStack.h"

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

// void sendMessage();                                                // Prototype
Task taskSendMessage(TASK_SECOND * 1.5, TASK_FOREVER, &sendMessage); // start with a one second interval

// Task to blink the number of nodes
Task blinkNoNodes;
bool onFlag = false;
String TrackerID = "";
float healthTimer = 0;

void setup()
{
  Serial.begin(115200);

  TrackerID = String(WiFi.macAddress());
  TrackerID = StringSeparator(TrackerID, ':', 0) + StringSeparator(TrackerID, ':', 1) + StringSeparator(TrackerID, ':', 2) + StringSeparator(TrackerID, ':', 3) +
              StringSeparator(TrackerID, ':', 4) + StringSeparator(TrackerID, ':', 4) + StringSeparator(TrackerID, ':', 5);
  Serial.print("TrackerID: ");
  Serial.println(TrackerID);
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
  getWeights();
  digitalWrite(LED, !onFlag);
}
String getTemperature()
{
  float tempValue = 23.2;

  return String(tempValue);
}
String getRSSI()
{
  return String(WiFi.RSSI());
}
float weights[16] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0};

void getWeights()
{
  //read weight sensors and upadate the global weights array
  //add real sensor values here, this function is called at approximate of 1/10th of a sec
  for (int i = 0; i < 15; i++)
  {
    weights[i] = (float)i;
  }

  //get sensor reading after every 1/10th of a second
}
String formatedWeights()
{

  String weightSamples = "";
  for (int i = 0; i < 15; i++)
  {
    weightSamples += String(weights[i]) + String("^");
  }

  return weightSamples;
}
void sendMessage()
{
  String msgV;
  healthTimer = healthTimer + 1;
  if (healthTimer < 20)
  {
    String msg = "" + TrackerID + String(";");
    msg += mesh.getNodeId();
    msg += ";";
    msg += String("w^") + formatedWeights();
    msgV = msg;
    mesh.sendBroadcast(msg); //after every 1.5 sec send the weight data
  }
  if (healthTimer > 20)
  {
    //after every 30 sec send temp and rssi data
    healthTimer = 0;
    String msg = "" + TrackerID + String(";");
    msg += mesh.getNodeId();
    msg += ";";
    msg += String("h^") + getTemperature() + String("^") + getRSSI();
    msgV = msg;
    mesh.sendBroadcast(msg);
  }

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

  Serial.printf("Sending message: %s\n", msgV.c_str());

  taskSendMessage.setInterval(TASK_SECOND * 1.5); // between 1 and 5 seconds
}

void receivedCallback(uint32_t from, String &msg)
{
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
  String msgRec = msg;
  //do whatever you want with the received msg here
  //like resetting the nodes etc.
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