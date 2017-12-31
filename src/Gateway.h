#pragma once

#include "HttpChannel.h"
#include "ZWaveNetwork.h"
#include "ptr.h"


///
/// HTTP to ZWave gateway
class Gateway : public HttpChannel {
public:
	
	///
	/// Constructor
	/// @param loop event loop for asynchronous io
	/// @param network a ZWave network to control over HTTP
	Gateway(asio::io_service & loop, ptr<ZWaveNetwork> network)
			: HttpChannel(loop, 30000), network(network) {
	}

	~Gateway() override;

	void onRequest(Method method, std::string url, Headers headers) override;
	void onBody(uint8_t const * data, size_t length) override;
	void onEnd() override;
	
	
	ptr<ZWaveNetwork> network;
	static std::map<std::string, std::string> defaultHeaders;
};
