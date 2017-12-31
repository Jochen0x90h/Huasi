#pragma once

#include <deque>
#include <chrono>
#include "asio.h"
#include "Object.h"
#include "ptr.h"


/// Get ZWave error category
error_category & getZWaveCategory();

///
/// ZWave protocol to a serial ZWave UZB1 dongle. Handles lowlevel request/response and ack/nack stuff
/// http://zwavepublic.com/specifications
class ZWaveProtocol : public Object {
public:
	// Message type
	enum MessageType {
		// start of frame
		SOF = 0x01,
		
		// acknowledge
		ACK = 0x06,
		
		// not acknowledge
		NACK = 0x15,
		
		// cancel - resend request
		CAN = 0x18
	};

	// Frame type
	enum FrameType {
		REQUEST = 0x00,
		RESPONSE = 0x01
	};


	// ZWave function
	// see OpenZWave, Defs.h (https://github.com/OpenZWave/open-zwave/blob/master/cpp/src/Defs.h)
	enum Function {
		///
		/// get version and list of nodes in the network
		/// see section 5.3.2.17 ZW_Version in "Z-Wave ZW0201/ZW0301 Appl. Prg. Guide v4.50"
		SERIAL_API_GET_INIT_DATA = 0x02,
		
		/// a node reports its status
		/// see section 5.3.1.5 ApplicationCommandHandler in "Z-Wave ZW0201/ZW0301 Appl. Prg. Guide v4.50"
		APPLICATION_COMMAND_HANDLER = 0x04,
		
		// send data to a node
		ZW_SEND_DATA = 0x13,
		
		// get node device class (without supported command classes)
		ZW_GET_NODE_PROTOCOL_INFO = 0x41,
		
		// report node device class and supported command classes, referred as Node Information Frame (NIF)
		// see section3.4 of "Z-Wave Application Command Class Specification"
		ZW_APPLICATION_UPDATE = 0x49,
		
		// request node info which will be sent in a APPLICATION_UPDATE request
		ZW_REQUEST_NODE_INFO = 0x60
	};


	///
	/// Request to the ZWave controller
	class Request : public Object {
	public:
		enum {MAX_REQUEST_LENGTH = 250};

		///
		/// Constructor
		/// @param function the ZWave FUNCTION in a request frame
		Request(uint8_t function) : function(function) {}
	
		virtual ~Request();
		
		///
		/// Get request and return the length which may be up to MAX_REQUEST_LENGTH
		/// @param data the data of the REQUEST frame: FRAME = SOF length REQUEST FUNCTION data [funcId] checksum
		virtual int getRequest(uint8_t * data) = 0;
		
		///
		/// Called when the response to a request was received
		/// @param data the data of the RESPONSE frame: : FRAME = SOF length RESPONSE FUNCTION data checksum
		virtual void onResponse(ZWaveProtocol * protocol, uint8_t const * data, int length) = 0;
	

		// ZWave FUNCTION (e.g. ZW_SEND_DATA)
		const uint8_t function;
	};

	///
	/// Request to send data over the network using a funcId
	class SendRequest : public Request {
		friend class ZWaveProtocol;
	public:
		SendRequest(uint8_t function) : Request(function) {}

		/// receive the additional response, a request that contains the funcId and typically a transmit status
		virtual void onRequest(ZWaveProtocol * protocol, uint8_t const * data, int length) = 0;

	private:
		// an id to identify the transmit status request which is actually a response to this request
		uint8_t funcId;
	};

		

	///
	/// Constructor
	/// @param loop event loop for asynchronous io
	/// @param device serial device of the zwave dongle
	ZWaveProtocol(asio::io_service & loop, std::string const & device);

	~ZWaveProtocol() override;

	///
	/// Send a request to the zwave controller and call request->onResponse() to receive the response
	void sendRequest(ptr<Request> request);

protected:

	///
	/// Received a request from the ZWave controller
	virtual void onRequest(uint8_t const * data, int length) = 0;
	
	///
	/// called when an error occurs
	virtual void onError(error_code error) = 0;

	enum Time {
		// wait for ACK timeout
		ACK_TIMEOUT = 1500
	};


	/// Start receiving data
	void receive();
	
	/// Send next request from the queue
	void sendRequest();

	/// Resend the current request after NACK or timeout
	/// @param error 1 for NACK and 2 for timeout
	void resendRequest(int error);

	/// Send acknowledge (after a frame was received with is ok)
	void sendAck();

	/// Send not acknowledge (after a frame was received with checksum error)
	void sendNack();

	/// Calculate checksum of zwave frame
	uint8_t calcChecksum(uint8_t const * buffer, int length);

	
	// serial connection to zwave dongle
	asio::serial_port tty;
	
	// queue of requests to be sent to the dongle
	std::deque<ptr<Request>> requests;
	
	// send buffer and timeout timer
	uint8_t txBuffer[256];
	asio::steady_timer txTimer;
	int txRetryCount;
	
	// recieve buffer
	int rxPosition;
	uint8_t rxBuffer[256];

	uint8_t nextFuncId = 1;
};
