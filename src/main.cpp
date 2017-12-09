#include <iostream>
#include "ZWaveNetwork.h"
#include "Channel.h"
#include "HttpChannel.h"
#include "Gateway.h"
#include "ptr.h"


class MyZWaveNetwork : public ZWaveNetwork {
public:
	MyZWaveNetwork(asio::io_service & service, std::string const & device)
			: ZWaveNetwork(service, device) {
	}

	void onError(error_code error) noexcept override {
		std::cout << "ZWaveNetwork::onError " << error.category().name() << ":" << error.message() << std::endl;
	}
};

class MyGateway : public Gateway {
public:
	MyGateway(asio::io_service & loop, ptr<ZWaveNetwork> network) : Gateway(loop, network) {
	}
	
	void onError(error_code error) noexcept override {
		std::cout << "Gateway::onError " << error.category().name() << ":" << error.message() << std::endl;
	}
};

class MyServer : public Server {
public:
	MyServer(asio::io_service & loop, asio::ip::tcp::endpoint const & endpoint, ptr<ZWaveNetwork> network)
			: Server(loop, endpoint), network(network) {
	}
	
	ptr<Channel> createChannel(asio::io_service & loop) noexcept override {
		return new MyGateway(loop, this->network);
	}

	virtual void onError(error_code error) noexcept override {
		std::cout << "Server::onError " << error.category().name() << ":" << error.message() << std::endl;
	}
	
	ptr<ZWaveNetwork> network;
};

int main(int argc, char ** argv) {
	if (argc < 2) {
		std::cout << "HTTP to ZWave gateway" << std::endl;
		std::cout << "usage: huasi zwave_serial_device [http_server_port]" << std::endl;
		return 1;
	}
	char const * device = argv[1];
	int port = argc <= 2 ? 8080 : atoi(argv[2]);
	
	// zwave network
	asio::io_service loop;
	ptr<ZWaveNetwork> network = new MyZWaveNetwork(loop, device);

	// http server
	ptr<MyServer> server = new MyServer(loop, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port), network);
	server->listen();

	// run event loop
	loop.run();
}
