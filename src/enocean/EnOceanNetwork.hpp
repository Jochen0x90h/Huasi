#pragma once

#include "EnOceanProtocol.hpp"
#include "cast.hpp"


///
/// Class for setting and tracking the state of the nodes in an EnOcean network
/// https://github.com/yepher/RaZBerry
class EnOceanNetwork : public EnOceanProtocol {
public:

	///
	/// Common command request
	class CommonCommandRequest : public Request {
	public:
		enum Code {
			RD_IDBASE = 8
		};
		
		CommonCommandRequest(Code code) : Request(COMMON_COMMAND), code(code) {}
		~CommonCommandRequest() override;
		int getData(uint8_t *data) override;
		int getOptionalData(uint8_t *data) override;
		void onResponse(EnOceanProtocol *protocol, const uint8_t *data, int length,
			const uint8_t *optionalData, int optionalLength) override;
	
		Code code;
	};




	///
	/// Constructor
	/// @param loop event loop for asynchronous io
	/// @param device serial device of the zwave dongle
	EnOceanNetwork(asio::io_service & service, std::string const & device);

	~EnOceanNetwork() override;

	///
	/// send parameters to a node
	/// @param nodeId id of node
	/// @param parameters parameters to set
	/// @return true if node exists in the ZWave network
	bool sendSet(uint32_t nodeId, const Parameters &parameters) override;
	
	///
	/// get tracked parameters of a node
	/// @param nodeId id of node
	/// @param parameters parameters to get
	/// @return true if node exists in the ZWave network
	bool get(uint32_t nodeId, Parameters &parameters) override;
	
protected:

	void onRequest(uint8_t packetType, const uint8_t *data, int length,
		const uint8_t *optionalData, int optionalLength) override;

};
