https://wiki.fhem.de/wiki/Z-Wave-ZME_UZB1_USB_Dongle
[[Kategorie:Z-Wave Components]]
# Huasi
A simple ZWave smart home server for the "ZME E|A|U|X UZB1" USB Z-Wave Transceiver. It has
only an HTTP interface to set and get the state of the nodes in the ZWave network. It is
written in C++11 with no external dependencies (except [asio](https://think-async.com/)
1.10.8 and [http-parser](https://github.com/nodejs/http-parser) which are included as 
headers) to make it easy to cross-compile for [OpenWRT](https://openwrt.org/), Raspberry PI 
etc.

The project is in an early stage so setup of a network (pairing) is not supported yet. Use
e.g. [FHEM](http://www.fhem.de/) for this.

## Usage
`huasi zwave_serial_device [http_server_port]`

zwave_serial_device
: Serial device where the ZWave dongle is connected, e.g. /dev/ttyUSB0 or
/dev/tty.usbmodem1411

http_server_port
: Port where the http server listens, default is 8080

## HTTP Interface
Set blinds and slat of jalousie at node 4:
`curl -X POST 'http://192.168.1.181:8080/node/4?position.blinds=50&position.slat=50'` 

Get state of jalousie at node 4: `curl http://127.0.0.1:8080/node/4`
Response: `position.blinds=50&position.slat=50`


## Supported Devices
- Fibaro Roller Shutter 2 (FGR-222)


## Installation

### OpenWRT
- See openwrt.cmake for build instructions
- Copy executable to root directory on OpenWRT
- In /etc/modules.d/usb-serial, add vid/pid of USB dongle (ZME UZB1): usbserial vendor=0x0658 product=0x0200
- Copy init.d/huasi to /etc/init.d/huasi
- /etc/init.d/huasi enable
- /etc/init.d/huasi start


# ZWave
This is a short introduction to the communication protocol between host and UZB USB Dongle. 
All two digit values are hex values. Also see
[public specifications](http://zwavepublic.com/specifications)

## Protocol
This section describes the communication protocol in an abstract view.

### Request to Controller
Send a request to the controller, e.g. get list of all nodes in the network:
```
send REQUEST
receive RESPONSE
```

### Request to Network
Send a request to send into the ZWave network, e.g. a command to a node to switch on a lamp:
```
send REQUEST (with funcId)
receive RESPONSE
receive REQUEST (contains funcId, used to filter double receive)
```

### Request from Network
A node reports that e.g. the state of a lamp or switch has changed:
```
receive REQUEST
```

## Messages
Messages are sent to and received from the USB Dongle via a serial port (e.g. on 
/dev/ttyUSB0 on Linux) with these settings 115200: bps, 8 bits, no parity, 1 stop bit.

### Frame
Structure of a standard frame (for list of FUNCTION values see
[OpenZWave](https://github.com/OpenZWave/open-zwave/blob/master/cpp/src/Defs.h)):
```
FRAME = SOF length TYPE FUNCTION data checksum
	SOF = 01 (start of frame)
	length (length of following data)
	TYPE
		REQUEST = 00
		RESPONSE = 01
	FUNCTION
		SERIAL_API_GET_INIT_DATA = 02 (get list of nodes in the network)
		APPLICATION_COMMAND_HANDLER = 04 (a node reports its status)
		ZW_SEND_DATA = 13 (send data to a node)
		ZW_GET_NODE_PROTOCOL_INFO = 41 (get node device class)
		ZW_APPLICATION_UPDATE = 49 (a node reports its device class and command classes, "NIF")
		ZW_REQUEST_NODE_INFO = 60 (request node info, ZW_APPLICATION_UPDATE follows)
		...
	data (payload data of frame)
	checksum (xor of frame bytes without first byte, negated)
```
Request Frame to send data to a node containing funcId:
```
FRAME = SOF length REQUEST FUNCTION data funcId checksum
	funcId = 01-ff (used to associate requests that are sent in response to requests that transfer data over the network)
```
Request Frame received from the controller containing funcId:
```
FRAME = SOF length REQUEST FUNCTION funcId data checksum
	funcId (same id that was used when sending a request to the controller)
```

### Acknowledgement
```
ACK = 06
NACK = 15
CAN = 18
```
Each FRAME is acknowledged with an ACK if the CHECKSUM is ok and with a NACK if there is a
checksum error. CAN means CANCEL and is sent when a next FRAME is sent while the other end 
expects an ACK.

### Send a Request
```
send REQUEST
receive ACK
```

### Receive a Frame (Request or Response)
```
receive FRAME
send ACK
```

### Send a Request with Checksum Error
```
send REQUEST
receive NACK
```

### Double send before ACK
```
send REQUEST
send REQUEST
receive CAN
```

## Node Info
Request node device class and supported command classes.
See 5.3.1.8 ApplicationControllerUpdate in "Z-Wave ZW0201/ZW0301 Appl. Prg. Guide v4.50"

Request for getting node info:
```
send REQUEST REQUEST_NODE_INFO nodeId
	nodeId (id of target node)
```
Response to request:
```
receive RESPONSE REQUEST_NODE_INFO RESULT
	RESULT
		SENT = 01
```
Request for receiving a node info from a node:
```
receive REQUEST APPLICATION_UPDATE STATUS nodeId length [BASIC GENERIC SPECIFIC CLASSES [CONTROL]]
	STATUS
		SUC_ID = 10
		DELETE_DONE = 20
		NEW_ID_ASSIGNED = 40
		ROUTING_PENDING = 80
		NODE_INFO_REQ_FAILED = 81
		NODE_INFO_REQ_DONE = 82
		NODE_INFO_RECEIVED = 84
	nodeId (id of source node)
	length (length of following data, may be 0)
	BASIC (basic device class)
	GENERIC (generic device class)
		CONTROLLER = 01
		STATIC_CONTROLLER = 02
		THERMOSTAT = 08
		SWITCH_BINARY = 10
		SWITCH_MULTILEVEL = 11
		SWITCH_TOGGLE = 13
		SENSOR_BINARY = 20
		SENSOR_MULTILEVEL = 21
		SENSOR_ALARM = a1
		...
	SPECIFIC (specific device class)
	CLASSES (list of supported command classes)
	CONTROL (list of command classes this device can control in other devices)
		MARK = ef (marks start of CONTROL list)		
```

### Example: Request node info from node 4 (Fibaro FGR-222)
```
send SOF length=05 REQUEST=00 REQUEST_NODE_INFO=60 nodeId=04
receive ACK
receive SOF length=04 RESPONSE=01 REQUEST_NODE_INFO=60 SENT=01
send ACK
receive SOF length=1c REQUEST=00 APPLICATION_UPDATE=49 STATUS.NODE_INFO_RECEIVED=84 nodeId=04 length=16 BASIC=04 GENERIC=11 SPECIFIC=06 CLASSES=(CONFIGURATION=70 SWITCH_BINARY=25 ...) CONTROL=(MARK=ef SWITCH_BINARY=25 ...)
send ACK
```

### Example: Request node info from node 1 (the controller)
Fails because it has no associated NIF
```
send SOF length=05 REQUEST=00 REQUEST_NODE_INFO=60 nodeId=01
receive ACK
receive SOF length=04 RESPONSE=01 REQUEST_NODE_INFO=60 SENT=01
send ACK
receive SOF length=06 REQUEST=00 APPLICATION_UPDATE=49 STATUS.FAILED=81 nodeId=00 length=00
send ACK
```

## Commands
Commands are used to tell nodes in the network to switch on the light, open the blinds or 
lock a door. Commands are also used to report the current state of the nodes, e.g. if a light 
or switch is on or the current temperature in a room.
See section 5.3.3.1 ZW_SendData and 5.3.1.5 ApplicationCommandHandler in "Z-Wave ZW0201/ZW0301 
Appl. Prg. Guide v4.50".
See section 3.2.1 in "Z-Wave Application Command Class Specification".

Request for sending a command to a node:
```
send REQUEST ZW_SEND_DATA nodeId COMMAND TX_OPTIONS funcId
	nodeId (id of target node)
	TX_OPTIONS (transmit options, bit-wise combination of the following)
		ACK = 01 (request transfer acknowledge from receiving node to ensure proper transmission)
		LOW_POWER = 02 (low transmit power (if node is closer than 2 meters)			
		AUTO_ROUTE = 04 (try to transmit via repeater nodes if direct transmission is not possible)
		NO_ROUTE = 10 (disable routing)			
		EXPLORE = 20 (support explorer frames for automatic fixing of the routing table)		
	funcId = id to identify the request that follows the response (see Protocol)
```
Response to request:
```
receive RESPONSE ZW_SEND_DATA RESULT
	RESULT
		SENT = 01
```
Additional request that follows the response that informs about transmit status:
```
receive REQUEST ZW_SEND_DATA funcId TX_STATUS ? ?
	TX_STATUS
		OK = 00
		NO_ACK = 01
		FAIL = 02
		NOT_IDLE = 03
		NOROUTE = 04
```
Request for receiving a command (status report) from a node:
```
receive REQUEST APPLICATION_COMMAND_HANDLER RX_STATUS nodeId COMMAND
	nodeId (id of source node)
	RX_STATUS (combination of the following flags)
		ROUTED_BUSY = 01
		LOW_POWER = 02 (received at low power)
		BROADCAST = 04 (received broadcast frame)
		MULTICAST = 08 (received multicast frame)
```
Structure of a command:
```
COMMAND = length CLASS ID data
	length (length of command without length itself)
	CLASS (command class 1 or 2 bytes)
		BASIC = 20
		CONFIGURATION = 70
		...
	ID (class specific command identifiers)
		GET
		SET
		REPORT
		...
	data (data of command)
```

### Example: Get BASIC value of node 4 (value is reported in an additional request)
```
send SOF length=09 REQUEST=00 ZW_SEND_DATA=13 node=04 COMMAND=(length=02 BASIC=20 GET=02) TX_OPTIONS=05 funcId=37
receive ACK
receive SOF length=04 RESPONSE=01 ZW_SEND_DATA=13 RESULT.SENT=01
send ACK
receive SOF length=07 REQUEST=00 ZW_SEND_DATA=13 funcId=37 TX_STATUS.OK=00 00? 02?
send ACK
receive SOF length=09 REQUEST=00 APPLICATION_COMMAND_HANDLER=04 RX_STATUS=00 nodeId=04 COMMAND=(length=03 BASIC=20 REPORT=03 value=01)
send ACK
```
