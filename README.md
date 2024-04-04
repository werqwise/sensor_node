<p align="center">
  <a href="" rel="noopener">
 <img width=200px height=200px src="artwork/sensio_mesh.png" alt="Project logo"></a>
</p>

<h3 align="center">SensioMesh</h3>

<div align="center">

[![Status](https://img.shields.io/badge/status-active-success.svg)]()


</div>

---


<p align="center"> SensioMesh
    <br> 
</p>

## 📝 Table of Contents

- [About](#about)
- [Getting Started](#getting_started)
- [Installing](#installing)
- [Mesh Architecture](#circuit)
- [Usage](#usage)
- [Built Using](#built_using)
- [Authors](#authors)


## 🧐 About <a name = "about"></a>

This repo contains firmware and configuration instructions for SensioMesh Projects.

## 🏁 Getting Started <a name = "getting_started"></a>

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See [deployment](#deployment) for notes on how to deploy the project on a live system.

### Prerequisites

What things you need to install the software and how to install them.

```
- Platform.io configured and running.
- Raspberry Pi with Raspberry Pi OS installed.
```

### Installing <a name = "installing"></a>

A step by step series that tell you how to get the Firmware and Backend running

#### Platform.io Configs

You should have Platform.io installed with the following pre-reqs

  1.  ESP32 Board should be installed in Platform.io
    

#### Sensor Nodes

Everything is already configured for Sensor Nodes Firmware and can be uploaded to the respective nodes for testing.

  1.  On line number 114 of MN_SensorNode(main.cpp) there is a function named `receivedCallback`. Whenever a data is being received by the Python script from MQTT, it is send to the Bridge Node and the Bridge Node broadcasts it to all the nodes in the mesh network. `receivedCallback` function is where the data is being received and msgRec is the final String varible which stores the received message which can be parsed and used for functionalities like node reset etc.
  2.  `sendMessage` on line number 90 of MN_SensorNode(main.cpp) is the function which is being called after every 1 second and broadcasts the node data to all the nodes in the network. Line number 95 contains a variable named msg which can be appended with any type of data like load sensors or temperature sensors in your case.
      1.  The final format of broadcasted message is in this format
        ```ESP32MAC Address;MeshNode ID;Any String Data ```

#### Bridge Node

Everything is already configured for the Bridge Node Firmware as well and can be uploaded to the respective node for testing.
 
  1.  On line number 91 of MN_BridgeNode there is a function `sendMessage`, which:
      1.  Sends the received data to the Raspberry Pi after every 0.5 seconds
      2.  Checks if there is an incomming data from the Raspberry Pi then:
          1.  Parses it
          2.  Broadcasts it to all the nodes in the mesh network
          3.  Sends ack message back to the Raspberry Pi
  2.  On line number 110 there is a function named `receivedCallback` which receives the data from all the nodes in the netowrk, parses it and then stores it in a custom-built data structure named `NodesHandler` which is in `nodesHandler.h` file.
  3.  Don't remove any Serial.print statement from Bridge Node firmware as it is used to communicate with the Raspberry Pi over USB Serial connection.

#### Gateway Node(Raspberry Pi)

To make the firmware ready for the Raspberry Pi

1.  Open the terminal and execute the following commands
    1.  sudo apt update
    2.  sudo apt upgrade
    3.  sudo pip3 install AWSIoTPythonSDK
    4.  sudo pip3 install --user pyserial
2. Then put the certificate files in certs folder and do the required changes in `mqttConfigs.py` file.
    1.  Copy PythonFirmware folder to your Raspberry Pis Desktop and run it using the following command
        ```python3 /Desktop/PythonFirmware/pyFirmware.py ```

##### Details

1.  The function named `broadcastDataToMesh` on line number 42 of `pyFirmware.py` file can be called anywhere to broadcast data to all the nodes in the network. For demo it is being called after every 30 seconds with temp data.
    1.  Always call the `broadcastDataToMesh` function in this format ```B:your Data```. 
2.  Line number 58 checks whether there's any incomming data and it contains any node information, if yes, it parses it within its block and publishes it to the MQTT broker.
3.  While line number 75 listens to the Acknowledgement messages from the Bridge Node.

## 🔧 Mesh Architecture <a name = "circuit"></a>

High level architecture of the Mesh network
![Circuit](artwork/des.png)


## 🎈 Usage <a name="usage"></a>

1.  Open mqttConfigs.py file
2.  Put your MQTT broker details there before running the firmware.
3.  Connect Bridge Node to any USB port of the Raspberry Pi and then run the Firmware.
4.  There could be unlimited number of sensor nodes in the mesh network.

## ⛏️ Built Using <a name = "built_using"></a>

- [Platform.io](https://platformio.org/) - Embedded Development Environment
- [PainlessMesh](https://gitlab.com/painlessMesh/painlessMesh/) - Mesh Development Framework
- [Python](https://www.python.org/) - Programming Language

## Demo Video
  - [MeshTracker](https://youtu.be/scoSdehjTmo) - SensioMesh Demo Video

## ✍️ Authors <a name = "authors"></a>

- [@Nauman3S](https://github.com/Nauman3S) - Development and Deployment

