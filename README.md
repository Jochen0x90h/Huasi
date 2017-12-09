Huasi
=====
A simple ZWave smart home server for the "ZME E|A|U|X UZB1" USB Z-Wave Transceiver. It has
only an HTTP interface to set and get the state of the nodes in the ZWave network. It is
written in C++11 with no external dependencies (except [asio][1] 1.10.8 and
[http-parser][2] which are included as headers) to make it easy to cross-compile for
[OpenWRT][3], Raspberry PI etc.

The project is in an early stage so setup of a network (pairing) is not supported yet. Use
e.g. [FHEM][4] for this.

Usage
-----
`huasi zwave_serial_device [http_server_port]`

zwave_serial_device
: Serial device where the ZWave dongle is connected, e.g. /dev/ttyUSB0 or
/dev/tty.usbmodem1411

http_server_port
: Port where the http server listens, default is 8080

HTTP Interface
--------------
Set blinds and slat of jalousie at node 4:
`curl -X POST 'http://192.168.1.181:8080/node/4?position.blinds=50&position.slat=50'` 

Get state of jalousie at node 4: `curl http://127.0.0.1:8080/node/4`
Response: `position.blinds=50&position.slat=50`


Supported Devices
--------------------------
- Fibaro Roller Shutter 2 (FGRM222)


[1]: https://think-async.com/
[2]: https://github.com/nodejs/http-parser
[3]: https://openwrt.org/
[4]: http://www.fhem.de/
