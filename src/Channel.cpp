#include <iostream>
#include "Channel.h"


Channel::Channel(asio::io_service & loop, int timeout)
		: socket(loop), timer(loop), timeout(timeout), txPending(0) {
}

Channel::~Channel() {
}

void Channel::sendData(uint8_t const * data, size_t length) {
	// add reference to this object until async_wait completes
	addReference();
	
	// restart timeout timer
	this->timer.expires_from_now(std::chrono::milliseconds(this->timeout));
	this->timer.async_wait([this] (error_code error) {
				if (!error) {
					onTimeout();
				}

				// remove reference to this object
				removeReference();
			});
	
	// add reference to this object until async_write completes
	addReference();

	// send data
	uint8_t * buffer = new uint8_t[length];
	std::copy(data, data + length, buffer);
	++this->txPending;
	asio::async_write(
			this->socket,
			asio::buffer(buffer, length),
			[this, buffer] (error_code error, size_t writtenCount) {
				delete [] buffer;
				--this->txPending;
				if (error)
					onError(error);
				
				// notify when new data is needed to be sent
				if (this->txPending == 0)
					onReadyToSend();

				// remove reference to this object
				removeReference();
			});
}

void Channel::shutdown() {
	this->socket.shutdown(asio::socket_base::shutdown_type::shutdown_send);
}

void Channel::close() {
	if (this->socket.is_open()) {
		// cancel timeout timer
		this->timer.cancel();
		
		// close socket (also cancels pending read/write requests)
		error_code error;
		this->socket.close(error);
		if (error)
			onError(error);
		
		// remove reference for the open socket, may delete this object
		removeReference();
	}
}

void Channel::onReadyToSend() {
}

void Channel::receive() {
	// add reference to this object until async_wait completes
	addReference();

	// restart timeout timer
	this->timer.expires_from_now(std::chrono::milliseconds(this->timeout));
	this->timer.async_wait([this] (error_code error) {
				if (!error) {
					onTimeout();
				}

				// remove reference to this object
				removeReference();
			});

	// add reference to this object until async_read_some completes
	addReference();

	// receive data
	this->socket.async_read_some(
			asio::buffer(this->rxBuffer, sizeof(this->rxBuffer)),
			[this] (error_code error, size_t readCount) {
				if (error) {
					if (isCanceled(error)) {
						//std::cout << "canceled" << std::endl;
					} else if (isEof(error)) {
						onShutdown();
					} else {
						onError(error);
					}
				} else {
					// continue receiving
					receive();

					onData(this->rxBuffer, readCount);
				}

				// remove reference to this object
				removeReference();
			});
}

void Channel::onShutdown() {
	close();
}

void Channel::onTimeout() {
	close();
}
