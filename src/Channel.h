#pragma once

#include <string>
#include <vector>
#include <system_error>
#include "Server.h"


///
/// TCP communication channel with inactivity timeout
class Channel : public Object {
	friend class Server;
public:
	///
	/// Constructor
	/// @param loop event loop for asynchronous io
	/// @param timeout inactivity timeout in milliseconds after which onTimeout() gets called
	Channel(asio::io_service & loop, int timeout);

	~Channel() override;
	
	///
	/// send data, only call from the event loop thread
	virtual void sendData(uint8_t const * data, size_t length) noexcept;
	
	///
	/// send string as data
	void sendData(std::string const & data) noexcept {sendData((uint8_t const *)data.data(), data.length());}
	
	///
	/// shutdown the connection which triggers onShutdown() on the other side
	/// (lowlevel: we call shutdown() for write which causes socket on other side to signal eof)
	void shutdown() noexcept;
	
	///
	/// close the channel. default implementation also deletes this channel
	virtual void close() noexcept;

protected:

	///
	/// called when a client or server connection was established. Receiving is already enabled
	virtual void onConnect() noexcept = 0;

	///
	/// called when channel became ready to send new data. When sending large amounts of data, first send a chunk of
	/// several kB and then wait for onReadyToSend() before sending the next chunk. default implementation does nothing
	virtual void onReadyToSend() noexcept;
	
	void receive() noexcept;
	
	///
	/// called when new data arrived
	virtual void onData(uint8_t const * data, size_t length) noexcept = 0;

	///
	/// called when channel was shut down or closed by peer. default implementation calls close()
	virtual void onShutdown() noexcept;

	///
	/// called when no data was sent or received for a given time. default imlementation calls close()
	virtual void onTimeout() noexcept;

	///
	/// called when an error occurs. Erros can be of categories dnsCategory(), tlsCategory(), std::system_category() or
	/// categories of derived classes such as HttpChannel. channel will be closed and deleted after onError returns
	virtual void onError(std::error_code error) noexcept = 0;


	asio::ip::tcp::socket socket;
	asio::steady_timer timer;
	int timeout;
	int txPending;
	uint8_t rxBuffer[4096];
};
