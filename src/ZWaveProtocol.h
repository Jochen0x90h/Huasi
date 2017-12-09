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
class ZWaveProtocol : public Object {
public:

	///
	/// request to the ZWave controller
	class Request : public Object {
	public:
		enum {MAX_REQUEST_LENGTH = 252};
	
		virtual ~Request();
		
		/// get request and return the length which may be up to MAX_REQUEST_LENGTH
		virtual int getRequest(uint8_t * data) noexcept = 0;
		
		/// receive the response for the request
		virtual void onResponse(uint8_t const * data, int length) noexcept = 0;
	};

	///
	/// Constructor
	/// @param loop event loop for asynchronous io
	/// @param device serial device of the zwave dongle
	ZWaveProtocol(asio::io_service & loop, std::string const & device);

	~ZWaveProtocol() override;

	///
	/// Send a request to the zwave controller and call request->onResponse() to receive the response
	void sendRequest(ptr<Request> request) noexcept;

protected:

	///
	/// Received a request from the ZWave controller
	virtual void onRequest(uint8_t const * data, int length) noexcept = 0;
	
	///
	/// called when an error occurs
	virtual void onError(error_code error) noexcept = 0;

	enum Time {
		// wait for ACK timeout
		ACK_TIMEOUT = 1500
	};

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

	enum FrameType {
		REQUEST = 0x00,
		RESPONSE = 0x01
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
};
