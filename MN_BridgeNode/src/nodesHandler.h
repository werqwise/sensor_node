class NodesHandler
{
private:
  const int size = 40;
  int NodesPointer = 0;
  String NodesList[40];
  String NodesData[40];
  String NodesID[40];

public:
  uint8_t addNode(String MAC, String data, String NodeID)
  {
    for (int i = 0; i < size; i++)
    {
      if (NodesList[i].indexOf(MAC) >= 0)
      {
        NodesData[i] = data;
        NodesID[i] = NodeID;
        ///  break;
        return 0;
      }
    }

    if (NodesPointer < size)
    {

      NodesList[NodesPointer] = MAC;
      NodesData[NodesPointer] = data;
      NodesID[NodesPointer] = NodeID;
      NodesPointer++;
      return 1;
    }
  }

  void removeNode(String nodeID)
  {

    for (int i = 0; i < size; i++)
    {
      String ss = NodesID[i];

      if (nodeID.indexOf(ss) >= 0)
      {
      }
      else
      {
        NodesData[i] = "NA";
      }
    }
  }
  int getNodesPointer()
  {
    return NodesPointer;
  }

  String getParsedData(int i)
  {
    if (i < size)
    {
      String v = "NodeMAC:";
      v = v + NodesList[i] + String(",NodeData:");
      v = v + NodesData[i] + String(",NodeID:");
      v = v + NodesID[i];
      return v;
    }
    else
    {
      return String("NA");
    }
  }
  void sendDataToUART(String temp)
  {
    String d = String(NodesPointer) + String("*");
    for (int i = 0; i < NodesPointer; i++)
    {
      d += (getParsedData(i));
      d = d + "*";
    }
    d = d + "&" + temp;
    Serial.println(d);
  }
  void printAll()
  {
    Serial.println("--------------");
    for (int i = 0; i < NodesPointer; i++)
    {
      Serial.print(getParsedData(i));
      String ss = StringSeparator(NodesData[i], ';', 1);
      Serial.print(ss);
      Serial.println();
    }
    Serial.print("Trackers: ");
    Serial.println(NodesPointer);
    Serial.println("--------------");
  }

  void printNodeIDs()
  {
    Serial.println("--------------");
    String ss;
    for (int i = 0; i < NodesPointer; i++)
    {

      ss = (NodesID[i]);
      Serial.print(ss);
      Serial.println();
    }
    Serial.print("Trackers: ");
    Serial.println(NodesPointer);
    Serial.println("--------------");
  }
};
