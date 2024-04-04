# Install the following pacakges on your RPi
# sudo pip3 install --user pyserial
# sudo pip3 install AWSIoTPythonSDK

import serial
import sys
import re
import datetime
import uuid
from serial import SerialException
try:
    from mqttHandler import *
except Exception as e:
    print('E: Check MQTT Configs')


serial_comm = serial.Serial()

# Configure the baud rate and it should be equal to the Arduino code serial communication baud rate
serial_comm.baudrate = 115200

# Configure the Serial port as needed '/dev/ttyXXXX' is applicable for Linux systems
# will be /dev/ttyUSB0 if no other USB peripheral is attached with raspberry pi
serial_comm.port = '/dev/ttyUSB0'

# Configure timeout seconds
serial_comm.timeout = 10

try:
    serial_comm.open()
except serial.SerialException:
    # Print an error when the serial communication isn't available
    print("Could not open port")
incommingData = ""
nodesList = []
numberOfNodes = 0
liveIDs = []
masterLiveMAC = []
masterLiveData = []
masterLiveID = []
masterLiveList = [masterLiveMAC, masterLiveData, masterLiveID]
g = 0


def encodeData(d):
    data = d
    data = data.encode()
    return data


def broadcastDataToMesh(data):  # use this function to broadcast data to all nodes
    global serial_comm
    g = encodeData(data)
    serial_comm.write(g)


def findAll(string, word):
    all_positions = []
    next_pos = -1
    while True:
        next_pos = string.find(word, next_pos+1)
        if(next_pos < 0):
            break
        all_positions.append(next_pos)
    return all_positions


def getIDList(string):
    lst = findAll(string, '"nodeId":')
    ids = []
    for i in range(0, len(lst)):
        ids.append(string[lst[i]+9:lst[i]+19])
    # print(ids)
    return ids


def nodeListPrinter(d1, d2, d3):
    print('{')
    print('NodeMAC: ', d1, ',')
    print('NodeData: ', d2, ',')
    print('NodeID: ', d3)
    print('}')


def formatHealthPacket(data):

    dataValue = '{\n "temperature": '+data[1]+',\n"RSSI": '+str(data[2])+'\n}'
    return dataValue


def formatWeightsPacket(data):
    g = ""
    timestamp = datetime.datetime.now()
    for i in range(1, 17):
        g = g+data[i]+','
    g = g[:-2]

    dataValue = '{\n "weight": ['+g+'],\n"timestamp": '+str(timestamp)+'\n}'
    return dataValue


def getPIMAC():
    mac = ''.join(re.findall('..', '%012x' % uuid.getnode()))
    return mac.upper()


while (True):
    if serial_comm.isOpen():
        try:
            print('TimeCounter:', g)
            if(g >= 30):  # sending something after every 30 seconds
                # add B: before your desired string
                broadcastDataToMesh('B:reset')
                g = 0
            g = g+1
            incommingData = serial_comm.readline().decode('utf-8')
            if('*' in incommingData):
                nodesList = (incommingData.split('*'))
                # print(nodesList)
                numberOfNodes = int(nodesList[0])
                print('Number of live Nodes in Mesh(excluding Bridge Node): ', str(
                    len(liveIDs)-1))

                if(numberOfNodes > 0):
                    liveIDs = getIDList(nodesList[len(nodesList)-1])
                    print('LiveIDs: BridgeNodeID - Other Node IDs...\n', liveIDs)

                    for i in range(1, numberOfNodes+1):
                        # 1*NodeMAC:240AC4AFDBDB9C,NodeData::253752* sample data format comming from node
                        nodeML = nodesList[i].split(',')
                        # print('nml',nodeML)
                        nodeMAC = nodeML[0].split(':')[1]
                        nodeData = nodeML[1].split(':')[1]
                        nodeID = nodeML[2].split(':')[1]
                        for i in range(0, len(liveIDs)):
                            if(liveIDs[i] in nodeID):
                                nodeListPrinter(nodeMAC, nodeData, nodeID)
                        # if(nodeMAC not in masterLiveMAC):
                                masterLiveMAC.append(nodeMAC)
                                masterLiveData.append(nodeData)
                                masterLiveID.append(nodeID)

                        try:

                            # publish data to node-macAddress topic for each nodes
                            if(nodeID in liveIDs):  # publish only if the node is live
                                dataPacketToPublish = ""
                                pubTopic = ""

                                dataV = nodeData.split('^')
                                print(len(dataV))
                                if(dataV[0] == 'w'):
                                    dataPacketToPublish = formatWeightsPacket(
                                        dataV)
                                    pubTopic = getPIMAC()+'/'+nodeMAC+'/WEIGHT'
                                    print('Publishing: ', dataPacketToPublish,
                                          ' to Topic: ', pubTopic)
                                    publishMQTTMessage(
                                        pubTopic, dataPacketToPublish)

                                elif(dataV[0] == 'h'):
                                    dataPacketToPublish = formatHealthPacket(
                                        dataV)
                                    pubTopic = getPIMAC()+'/'+nodeMAC+'/HEALTH'
                                    print('Publishing: ', dataPacketToPublish,
                                          ' to Topic: ', pubTopic)
                                    publishMQTTMessage(
                                        pubTopic, dataPacketToPublish)

                        except Exception as e:
                            print('')
            elif('ACK' in incommingData):
                print('Acknowledgement Received: ', incommingData)
        except Exception as e:
            print('e: ', e)
    else:
        print("Exiting")
        break
serial_comm.close()
sys.exit()
