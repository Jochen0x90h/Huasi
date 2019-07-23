#pragma once

#include <deque>
#include <chrono>
#include "asio.hpp"
#include "Network.hpp"
#include "ptr.hpp"


/// Get ZWave error category
error_category & getEnOceanCategory();

///
/// EnOcean protocol to a serial USB300/TCM310 dongle. Handles lowlevel request/response and ack/nack stuff
/// https://www.enocean.com/fileadmin/redaktion/pdf/tec_docs/EnOceanSerialProtocol3.pdf
class EnOceanProtocol : public Network {
public:
	// Packet type
	enum PacketType {
		// ERP1 radio telegram (raw data)
		RADIO_ERP1 = 1,
		
		//
		RESPONSE = 2,
		
		//
		RADIO_SUB_TEL = 3,
		
		// An EVENT is primarily a confirmation for processes and procedures, incl. specific data content
		EVENT = 4,
		
		// Common commands
		COMMON_COMMAND = 5,
		
		//
		SMART_ACK_COMMAND = 6,
		
		//
		REMOTE_MAN_COMMAND = 7,
		
		//
		RADIO_MESSAGE = 9,
		
		//
		RADIO_ERP2 = 10,
		
		//
		RADIO_802_15_4 = 16,
		
		//
		COMMAND_2_4 = 17
	};

	// Limits
	enum {
		MAX_DATA_LENGTH = 256,
		MAX_OPTIONAL_LENGTH = 16
	};

	///
	/// Request to the EnOcean controller
	class Request : public Object {
	public:

		///
		/// Constructor
		/// @param packetType the packet type to send
		Request(uint8_t packetType) : packetType(packetType) {}
	
		virtual ~Request();
		
		///
		/// Get data and return the length which may be up to MAX_DATA_LENGTH
		/// @param data the data of the frame
		virtual int getData(uint8_t *data) = 0;

		///
		/// Get optional data and return the length which may be up to MAX_OPTIONAL_LENGTH
		/// @param data the optional data of the frame
		virtual int getOptionalData(uint8_t *data) = 0;

		///
		/// Called when the response to a request was received
		/// @param data the data of the RESPONSE frame: : FRAME = SOF length RESPONSE FUNCTION data checksum
		virtual void onResponse(EnOceanProtocol *protocol, const uint8_t *data, int length,
			const uint8_t *optionalData, int optionalLength) = 0;
	

		// Packet type (e.g. COMMON_COMMAND)
		const uint8_t packetType;
	};

	

	///
	/// Constructor
	/// @param loop event loop for asynchronous io
	/// @param device serial device of the EnOcean controller (e.g. "/dev/tty.usbserial-FT3PMLOR")
	EnOceanProtocol(asio::io_service &loop, const std::string &device);

	~EnOceanProtocol() override;

	///
	/// Send a request to the EnOcean controller. When the response arrives, request->onResponse() gets called
	void sendRequest(ptr<Request> request);

protected:

	///
	/// Received a request from the EnOcean controller
	virtual void onRequest(uint8_t packetType, const uint8_t *data, int length,
		const uint8_t *optionalData, int optionalLength) = 0;
	
	///
	/// Called when an error occurs
	virtual void onError(error_code error) = 0;

	enum Time {
		// time to wait for a RESPONSE from the EnOcean controller
		RESPONSE_TIMEOUT = 1500
	};


	/// Start receiving data
	void receive();
	
	/// Send next request from the queue
	void sendRequest();

	/// Resend the current request after NACK or timeout
	/// @param error 1 for NACK and 2 for timeout
	void resendRequest(int error);


	/// Calculate checksum of EnOcean frame
	uint8_t calcChecksum(const uint8_t *data, int length);

	
	// serial connection to zwave dongle
	asio::serial_port tty;
	
	// queue of requests to be sent to the dongle
	std::deque<ptr<Request>> requests;
	
	// send buffer and timeout timer
	uint8_t txBuffer[6 + MAX_DATA_LENGTH + MAX_OPTIONAL_LENGTH + 1];
	asio::steady_timer txTimer;
	int txRetryCount = 0;
	
	// recieve buffer
	int rxPosition = 0;
	uint8_t rxBuffer[6 + MAX_DATA_LENGTH + MAX_OPTIONAL_LENGTH + 1];
};
