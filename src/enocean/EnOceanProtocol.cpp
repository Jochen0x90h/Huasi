#include <iostream>
#include <iomanip>
#include "cast.hpp"
#include "EnOceanProtocol.hpp"

using spb = asio::serial_port_base;


// EnOcean error category
class EnOceanCategory : public error_category {
public:
	const char *name() const noexcept override {
		return "enocean";
	}
	
	std::string message(int val) const override {
		switch (val) {
		case 1:
			return "send_request_nack";
		case 2:
			return "send_request_timeout";
		}
		return std::string();
	}
};
static EnOceanCategory enOceanCategory;
error_category & getEnOceanCategory() {
	return enOceanCategory;
}

#ifdef DEBUG_PROTOCOL
namespace {
	void printFrame(uint8_t const * data, int length, int funcIdPos) {
	}
}
#endif


// Request

EnOceanProtocol::Request::~Request() {
}


// EnOceanProtocol

EnOceanProtocol::EnOceanProtocol(asio::io_service &loop, const std::string &device)
	: tty(loop), txTimer(loop)
{
	this->tty.open(device);
	
	// set 115200 8N1
	this->tty.set_option(spb::baud_rate(57600));
	this->tty.set_option(spb::character_size(8));
	this->tty.set_option(spb::parity(spb::parity::none));
	this->tty.set_option(spb::stop_bits(spb::stop_bits::one));
	this->tty.set_option(spb::flow_control(spb::flow_control::none));

	// start receiving data
	receive();
}

EnOceanProtocol::~EnOceanProtocol() {
}

void EnOceanProtocol::sendRequest(ptr<Request> request) {
	this->requests.push_back(request);
	if (this->requests.size() == 1) {
		// sent request immediately if request queue was empty
		sendRequest();
	}
}

void EnOceanProtocol::receive() {
	this->tty.async_read_some(
			asio::buffer(this->rxBuffer + this->rxPosition, sizeof(this->rxBuffer) - this->rxPosition),
			[this] (error_code error, size_t readCount) {
				if (error) {
					onError(error);
					return;
				}
				this->rxPosition += readCount;
				
				// check if header is complete
				while (this->rxPosition > 6) {
					// check if header is ok
					if (this->rxBuffer[0] == 0x55 && calcChecksum(this->rxBuffer + 1, 4) == this->rxBuffer[5]) {
						int dataLength = (this->rxBuffer[1] << 8) | this->rxBuffer[2];
						int optionalLength = this->rxBuffer[3];
						int length = dataLength + optionalLength;
						uint8_t packetType = this->rxBuffer[4];
					
						// check if complete frame has arrived
						if (this->rxPosition >= 6 + length + 1) {
							// check if frame is ok
							if (calcChecksum(this->rxBuffer + 6, length) == this->rxBuffer[6 + length]) {
								// cancel timeout and reset retry count
								this->txTimer.cancel();
								this->txRetryCount = 0;

								// check if this is a response to a pending request
								if (packetType == RESPONSE && !this->requests.empty()) {
									this->requests.front()->onResponse(this, this->rxBuffer + 6, dataLength,
										this->rxBuffer + 6 + dataLength, optionalLength);
								
									// remove request from queue
									this->requests.pop_front();
						
									// send next request if there is one in the queue
									if (!this->requests.empty())
										sendRequest();
								} else {
									onRequest(packetType, this->rxBuffer + 6, dataLength,
										this->rxBuffer + 6 + dataLength, optionalLength);
								}
								
								// remove frame from buffer
								std::move(this->rxBuffer + 6 + length + 1, this->rxBuffer + this->rxPosition,
									this->rxBuffer);
								this->rxPosition -= 6 + length + 1;
							} else {
								// error
								#ifndef NDEBUG
								std::cout << "receive corrupt frame" << std::endl;
								#endif
								
								this->rxPosition = 0;
							}
						} else {
							// incomplete frame: continue receiving
							break;
						}
					} else {
						// error
						#ifndef NDEBUG
						std::cout << "receive corrupt frame" << std::endl;
						#endif
						
						this->rxPosition = 0;
					}
				}
		
				// continue receiving
				receive();
			});
}

void EnOceanProtocol::sendRequest() {
	ptr<Request> request = this->requests.front();

	// get data from request
	int dataLength = request->getData(this->txBuffer + 6);
	
	// get optional data from request
	int optionalLength = request->getOptionalData(this->txBuffer + 6 + dataLength);

	int length = dataLength + optionalLength;
	this->txBuffer[0] = 0x55;
	this->txBuffer[1] = dataLength >> 8;
	this->txBuffer[2] = dataLength;
	this->txBuffer[3] = optionalLength;
	this->txBuffer[4] = request->packetType;
	this->txBuffer[5] = calcChecksum(this->txBuffer + 1, 4);
	this->txBuffer[6 + length] = calcChecksum(this->txBuffer + 6, length);

	// start timeout timer
	this->txTimer.expires_from_now(std::chrono::milliseconds(RESPONSE_TIMEOUT));
	this->txTimer.async_wait([this] (error_code error) {
		if (!error) {
			// timer expired before response was received
			resendRequest(2);
		}
	});
	
	// send request
	#ifdef DEBUG_PROTOCOL
	std::cout << "send";
	printFrame(this->txBuffer, 6 + length);
	#endif
	asio::async_write(
		this->tty,
		asio::buffer(this->txBuffer, 6 + length + 1),
		[this] (error_code error, size_t writtenCount) {
			if (error) {
				onError(error);
			} else {
				// wait for response or timeout
			}
		});
}

void EnOceanProtocol::resendRequest(int error) {
	if (++this->txRetryCount < 3) {
		// send request again
		sendRequest();
	} else {
		// reset retry count
		this->txRetryCount = 0;

		// remove request from queue
		ptr<Request> request = this->requests.front();
		this->requests.pop_front();
		
		// inform of error and delete
		onError(error_code(error, enOceanCategory));
	}
}


static const uint8_t crc8Table[256] = {0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15, 0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d, 0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65, 0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d, 0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5, 0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd, 0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85, 0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd, 0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2, 0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea, 0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2, 0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a, 0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32, 0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a, 0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42, 0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a, 0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c, 0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4, 0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec, 0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4, 0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c, 0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44, 0x19, 0x1e, 0x17, 0x10, 0x05, 0x02, 0x0b, 0x0c, 0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34, 0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b, 0x76, 0x71, 0x78, 0x7f, 0x6A, 0x6d, 0x64, 0x63, 0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b, 0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13, 0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb, 0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8D, 0x84, 0x83, 0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb, 0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3};

uint8_t EnOceanProtocol::calcChecksum(const uint8_t *data, int length) {
	uint8_t crc = 0;
	for (int i = 0 ; i < length ; ++i)
		crc = crc8Table[crc ^ data[i]];
	return crc;
}
