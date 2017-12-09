#include <iostream>
#include <iomanip>
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

void ZWaveProtocol::sendRequest(ptr<Request> request) noexcept {
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
								uint8_t checksum = calcChecksum(this->rxBuffer + 1, length);
								if (this->rxBuffer[2 + length - 1] == checksum) {
									// checksum is ok
									uint8_t frameType = this->rxBuffer[2];
									//std::cout << "receive " << (frameType == REQUEST ? "REQUEST" : "RESPONSE") << std::endl;
									
									// only one frame may be received at a time, mutiple only because other side didn't
									// receive ACK in time 
									if (!frame) {
										frame = true;
										
										// send ACK
										//std::cout << "send ACK" << std::endl;
										sendAck();
										
										if (frameType == REQUEST) {
											onRequest(this->rxBuffer + 3, length - 2);
										} else if (frameType == RESPONSE) {
											if (!this->requests.empty()) {
												// cancel timeout and reset retry count (if ACK was missing)
												this->txTimer.cancel();
												this->txRetryCount = 0;

												// remove request from queue
												ptr<Request> request = this->requests.front();
												this->requests.pop_front();
												
												// send next request if there is one in the queue
												if (!this->requests.empty())
													sendRequest();

												// notify response (may generate new requests) and delete
												request->onResponse(this->rxBuffer + 3, length - 2);
											}
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
						//std::cout << "receive ACK" << std::endl;

						// remove ACK from rxBuffer
						std::copy(this->rxBuffer + 1, this->rxBuffer + this->rxPosition, this->rxBuffer);
						--this->rxPosition;
						
						// cancel timeout and reset retry count
						this->txTimer.cancel();
						this->txRetryCount = 0;
						
						// wait for response
					} else if (messageType == NACK) {
						//std::cout << "receive NACK" << std::endl;

						// remove NACK from rxBuffer
						std::copy(this->rxBuffer + 1, this->rxBuffer + this->rxPosition, this->rxBuffer);
						--this->rxPosition;

						// cancel timeout
						this->txTimer.cancel();

						// resend request
						if (!this->requests.empty())
							resendRequest(1);
					} else if (messageType == CAN) {
						//std::cout << "receive CAN" << std::endl;

						// remove CAN from rxBuffer
						std::copy(this->rxBuffer + 1, this->rxBuffer + this->rxPosition, this->rxBuffer);
						--this->rxPosition;
					} else {
						// unknown
						//std::cout << "receive unknown message" << std::endl;
						
						this->rxPosition = 0;
					}
				}
		
				// continue receiving
				receive();
			});
}

void ZWaveProtocol::sendRequest() {
	int length = this->requests.front()->getRequest(this->txBuffer + 3);
	this->txBuffer[0] = SOF;
	this->txBuffer[1] = length + 2;
	this->txBuffer[2] = REQUEST;
	this->txBuffer[3 + length] = calcChecksum(this->txBuffer + 1, 2 + length);
	
	// start timeout timer
	this->txTimer.expires_from_now(std::chrono::milliseconds(ACK_TIMEOUT));
	this->txTimer.async_wait([this] (error_code error) {
		if (!error) {
			// timer expired before ACK or NACK was received
			resendRequest(2);
		}
	});
	
	// send request
	//std::cout << "send REQUEST" << std::endl;
	asio::async_write(
			this->tty,
			asio::buffer(this->txBuffer, 3 + length + 1),
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
