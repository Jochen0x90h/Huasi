#pragma once

#include <string>
#include <system_error>
#include "asio.hpp"
#include "Object.hpp"
#include "ptr.hpp"


class Channel;

///
/// tcp server that listens on a socket and uses createChannel() for connecting clients
class Server : public Object {
	friend class Channel;
public:
	///
	/// creates a new channel
	/// @param loop an asio event loop
	/// @param ipv4 or ipv6 address
	Server(asio::io_service & loop, asio::ip::tcp::endpoint const & endpoint);

	~Server() override;

	///
	/// listen for incoming connections
	bool listen();

	///
	/// close the server which will cause the destructor to be called at a later time
	void close();

protected:

	///
	/// create a channel that is used for an incoming connection
	virtual ptr<Channel> createChannel(asio::io_service & loop) = 0;

	///
	/// called when an error occurs
	virtual void onError(std::error_code error) = 0;


	// libuv callbacks
	//static void on_connect(uv_stream_t *handle, int status);
	//static void on_closed(uv_handle_t *handle);

	// server socket
	asio::ip::tcp::acceptor acceptor;
};
