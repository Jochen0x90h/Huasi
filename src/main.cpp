#include <iostream>
#include "zwave/ZWaveNetwork.hpp"
#include "enocean/EnOceanNetwork.hpp"
#include "http/HttpChannel.hpp"
#include "Gateway.hpp"
#include "ptr.hpp"


class MyZWaveNetwork : public ZWaveNetwork {
public:
	MyZWaveNetwork(asio::io_service &service, const std::string &device)
		: ZWaveNetwork(service, device) {
	}

	void onError(error_code error) noexcept override {
		std::cout << "ZWaveNetwork::onError " << error.category().name() << ":" << error.message() << std::endl;
	}
};

class MyEnOceanNetwork : public EnOceanNetwork {
public:
	MyEnOceanNetwork(asio::io_service &service, const std::string &device)
		: EnOceanNetwork(service, device) {
	}

	void onError(error_code error) noexcept override {
		std::cout << "EnOceanNetwork::onError " << error.category().name() << ":" << error.message() << std::endl;
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

// server that accepts connections and creates a MyGateway instance for every incoming connection
class MyServer : public Server {
public:
	MyServer(asio::io_service & loop, const asio::ip::tcp::endpoint &endpoint, ptr<ZWaveNetwork> network)
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
	
	// event loop
	asio::io_service loop;
	
	// ZWave network
	ptr<ZWaveNetwork> network = new MyZWaveNetwork(loop, device);

	// EnOcean network
	//ptr<EnOceanNetwork> network = new MyEnOceanNetwork(loop, device);

	// http server
	ptr<MyServer> server = new MyServer(loop, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port), network);
	server->listen();

	// run event loop
	loop.run();
}
