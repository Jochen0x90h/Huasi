#include <assert.h>
#include "Channel.h"
#include "ptr.h"


Server::Server(asio::io_service & loop, asio::ip::tcp::endpoint const & endpoint) : acceptor(loop, endpoint) {
}

Server::~Server() {
}

bool Server::listen() {
	// create a channel
	ptr<Channel> channel = createChannel(this->acceptor.get_io_service());
	
	// add reference to this object until async_accept completes
	addReference();
	this->acceptor.async_accept(
			channel->socket,
			[this, channel] (error_code error) {
				if (error) {
					onError(error);
				} else {
					// add a reference to the channel that keeps the channel alive until it is closed
					channel->addReference();

					// notify established connection
					channel->onConnect();
					
					// start receiving
					channel->receive();
				}
				
				// listen for the next incoming connection
				listen();
				
				// remove reference to this object
				removeReference();
			});
	return true;
}

void Server::close() {
	error_code error;
	this->acceptor.close(error);
	if (error)
		onError(error);
}
