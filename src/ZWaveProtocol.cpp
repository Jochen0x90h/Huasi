#include <iostream>
#include <iomanip>
#include "cast.h"
#include "ZWaveProtocol.h"

using spb = asio::serial_port_base;


// zwave error category
class ZWaveCategory : public error_category {
public:
	const char *name() const noexcept override {
		return "zwave";
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
static ZWaveCategory zwaveCategory;
error_category & getZWaveCategory() {
	return zwaveCategory;
}

#ifdef DEBUG_PROTOCOL
namespace {
	void printFrame(uint8_t const * data, int length, int funcIdPos) {
		std::cout << " SOF length=" << std::hex << std::setfill('0') << std::setw(2) << int(data[1]) << ' ';
	
		// frame type
		switch (data[2]) {
		case ZWaveProtocol::REQUEST:
			std::cout << "REQUEST=";
			break;
		case ZWaveProtocol::RESPONSE:
			std::cout << "RESPONSE=";
			break;
		}
		std::cout << std::setfill('0') << std::setw(2) << int(data[2]) << ' ';
		
		// function
		switch (data[3]) {
		case ZWaveProtocol::SERIAL_API_GET_INIT_DATA:
			std::cout << "SERIAL_API_GET_INIT_DATA=";
			break;
		case ZWaveProtocol::APPLICATION_COMMAND_HANDLER:
			std::cout << "APPLICATION_COMMAND_HANDLER=";
			break;
		case ZWaveProtocol::ZW_SEND_DATA:
			std::cout << "ZW_SEND_DATA=";
			break;
		case ZWaveProtocol::ZW_GET_NODE_PROTOCOL_INFO:
			std::cout << "ZW_GET_NODE_PROTOCOL_INFO=";
			break;
		case ZWaveProtocol::ZW_APPLICATION_UPDATE:
			std::cout << "ZW_APPLICATION_UPDATE=";
			break;
		case ZWaveProtocol::ZW_REQUEST_NODE_INFO:
			std::cout << "ZW_REQUEST_NODE_INFO=";
			break;
		}
		std::cout << std::setfill('0') << std::setw(2) << int(data[3]);
		
		for (int i = 4; i < length; ++i) {
			std::cout << ' ';
			if (i == funcIdPos)
				std::cout << "funcId=";
			std::cout << std::setfill('0') << std::setw(2) << int(data[i]);
		}
		std::cout << std::dec << std::endl;
	}
}
#endif


// Request

ZWaveProtocol::Request::~Request() {
}


// ZWaveProtocol

ZWaveProtocol::ZWaveProtocol(asio::io_service & loop, std::string const & device)
		: tty(loop), txTimer(loop), txRetryCount(0), rxPosition(0) {
	this->tty.open(device);
	
	// set 115200 8N1
	this->tty.set_option(spb::baud_rate(115200));
	this->tty.set_option(spb::character_size(8));
	this->tty.set_option(spb::parity(spb::parity::none));
	this->tty.set_option(spb::stop_bits(spb::stop_bits::one));
	this->tty.set_option(spb::flow_control(spb::flow_control::none));

	// start receiving data
	receive();
}

ZWaveProtocol::~ZWaveProtocol() {
}

void ZWaveProtocol::sendRequest(ptr<Request> request) {
	this->requests.push_back(request);
	if (this->requests.size() == 1) {
		sendRequest();
	}
}

void ZWaveProtocol::receive() {
	this->tty.async_read_some(
			asio::buffer(this->rxBuffer + this->rxPosition, sizeof(this->rxBuffer) - this->rxPosition),
			[this] (error_code error, size_t readCount) {
				if (error) {
					onError(error);
					return;
				}
				this->rxPosition += readCount;
				
				bool frame = false;
				while (this->rxPosition > 0) {
					uint8_t messageType = this->rxBuffer[0];
					if (messageType == SOF) {
						if (this->rxPosition >= 3) {
							// check if frame is complete
							int length = this->rxBuffer[1];
							if (this->rxPosition >= 2 + length) {
								// check if checksum is ok
								uint8_t checksum = calcChecksum(this->rxBuffer + 1, length);
								if (this->rxBuffer[2 + length - 1] == checksum) {
									// only one frame may be received at a time, mutiple only because other side didn't
									// receive ACK in time 
									if (!frame) {
										frame = true;

										uint8_t frameType = this->rxBuffer[2];
										uint8_t function = this->rxBuffer[3];
										#ifdef DEBUG_PROTOCOL
										std::cout << "receive";
										int funcIdPos = 0;
										if (!this->requests.empty() && this->requests.front()->function == function) {
											ptr<Request> request = this->requests.front();
											ptr<SendRequest> sr = cast<SendRequest>(request);
											if (frameType == REQUEST && sr != nullptr && sr->funcId == this->rxBuffer[4])
												funcIdPos = 4;
										}
										printFrame(this->rxBuffer, 1 + length, funcIdPos);
										#endif
										
										// send ACK
										sendAck();
										
										// check if the function matches a pending request
										if (!this->requests.empty() && this->requests.front()->function == function) {
											ptr<Request> request = this->requests.front();
											ptr<SendRequest> sr = cast<SendRequest>(request);
											
											bool isResponse = frameType == RESPONSE;
											bool isRequest = frameType == REQUEST && sr != nullptr && sr->funcId == this->rxBuffer[4];
											
											// check for end of request procedure
											if ((isResponse && sr == nullptr) || isRequest) {
												// cancel timeout and reset retry count (if ACK was missing)
												this->txTimer.cancel();
												this->txRetryCount = 0;

												// remove request from queue
												this->requests.pop_front();
												
												// send next request if there is one in the queue
												if (!this->requests.empty())
													sendRequest();
											}
											
											if (isResponse) {
												// notify response (may generate new requests)
												// omit SOF, length, REQUEST, FUNCTION and checksum
												request->onResponse(this, this->rxBuffer + 4, length - 3);
											} else if (isRequest) {
												// notify request
												// omit SOF, length, REQUEST, FUNCTION and checksum
												sr->onRequest(this, this->rxBuffer + 4, length - 3);
											}
										} else if (frameType == REQUEST) {
											// received a request that is not part of a request/response procedure
											// omit SOF, length, REQUEST and checksum
											onRequest(this->rxBuffer + 3, length - 2);
										}
									}
									
									// remove frame from buffer
									std::copy(this->rxBuffer + 2 + length, this->rxBuffer + this->rxPosition,
											this->rxBuffer);
									this->rxPosition -= 2 + length;
								} else {
									// checksum error: send NACK and hope controller will repeat
									sendNack();
									this->rxPosition = 0;
								}
							} else {
								// incomplete frame: continue receiving
								break;
							}
						} else {
							// incomplete frame: continue receiving
							break;
						}
					} else if (messageType == ACK) {
						#ifdef DEBUG_PROTOCOL
						std::cout << "receive ACK" << std::endl;
						#endif
						
						// remove ACK from rxBuffer
						std::copy(this->rxBuffer + 1, this->rxBuffer + this->rxPosition, this->rxBuffer);
						--this->rxPosition;
						
						// cancel timeout and reset retry count
						this->txTimer.cancel();
						this->txRetryCount = 0;
						
						// wait for response
					} else if (messageType == NACK) {
						#ifdef DEBUG_PROTOCOL
						std::cout << "receive NACK" << std::endl;
						#endif

						// remove NACK from rxBuffer
						std::copy(this->rxBuffer + 1, this->rxBuffer + this->rxPosition, this->rxBuffer);
						--this->rxPosition;

						// cancel timeout
						this->txTimer.cancel();

						// resend request
						if (!this->requests.empty())
							resendRequest(1);
					} else if (messageType == CAN) {
						#ifdef DEBUG_PROTOCOL
						std::cout << "receive CAN" << std::endl;
						#endif

						// remove CAN from rxBuffer
						std::copy(this->rxBuffer + 1, this->rxBuffer + this->rxPosition, this->rxBuffer);
						--this->rxPosition;
					} else {
						// unknown
						#ifndef NDEBUG
						std::cout << "receive unknown message" << std::endl;
						#endif
						
						this->rxPosition = 0;
					}
				}
		
				// continue receiving
				receive();
			});
}

void ZWaveProtocol::sendRequest() {
	ptr<Request> request = this->requests.front();
	
	int length = 4 + request->getRequest(this->txBuffer + 4);
	if (ptr<SendRequest> sr = cast<SendRequest>(request)) {
		// set funcId to request and add to buffer
		this->txBuffer[length] = sr->funcId = this->nextFuncId;
		this->nextFuncId = this->nextFuncId < 0xff ? this->nextFuncId + 1 : 1;
		++length;
	}
	this->txBuffer[0] = SOF;
	this->txBuffer[1] = length - 1;
	this->txBuffer[2] = REQUEST;
	this->txBuffer[3] = request->function;
	this->txBuffer[length] = calcChecksum(this->txBuffer + 1, length - 1);
	
	// start timeout timer
	this->txTimer.expires_from_now(std::chrono::milliseconds(ACK_TIMEOUT));
	this->txTimer.async_wait([this] (error_code error) {
		if (!error) {
			// timer expired before ACK or NACK was received
			resendRequest(2);
		}
	});
	
	// send request
	#ifdef DEBUG_PROTOCOL
	std::cout << "send";
	printFrame(this->txBuffer, length, isa<SendRequest>(request) ? length - 1 : 0);
	#endif
	asio::async_write(
			this->tty,
			asio::buffer(this->txBuffer, length + 1),
			[this] (error_code error, size_t writtenCount) {
				if (error) {
					onError(error);
				} else {
					// wait for timeout, ACK or NACK
				}
			});
}

void ZWaveProtocol::resendRequest(int error) {
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
		onError(error_code(error, zwaveCategory));
	}
}

void ZWaveProtocol::sendAck() {
	#ifdef DEBUG_PROTOCOL
	std::cout << "send ACK" << std::endl;
	#endif
	static const uint8_t data[] = {ACK};
	asio::async_write(
			this->tty,
			asio::buffer(data),
			[this] (error_code error, size_t writtenCount) {
				if (error) {
					onError(error);
				}
			});
}

void ZWaveProtocol::sendNack() {
	#ifdef DEBUG_PROTOCOL
	std::cout << "send NACK" << std::endl;
	#endif
	static const uint8_t data[] = {NACK};
	asio::async_write(
			this->tty,
			asio::buffer(data),
			[this] (error_code error, size_t writtenCount) {
				if (error) {
					onError(error);
				}
			});
}

uint8_t ZWaveProtocol::calcChecksum(uint8_t const * buffer, int length) {
    uint8_t checksum = 0xff;
    for (int i = 0; i < length; ++i) {
        // xor bytes
        checksum ^= buffer[i];
    }
    return checksum;
}
