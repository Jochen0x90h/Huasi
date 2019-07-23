#pragma once

#include "Parameters.hpp"
#include "Object.hpp"


///
/// Base class for smart home network, e.g. ZWave or EnOcean
class Network : public Object {
public:

	~Network() override;

	///
	/// send parameters to a node
	/// @param nodeId id of node
	/// @param parameters parameters to set
	/// @return true if node exists in the ZWave network
	virtual bool sendSet(uint32_t nodeId, const Parameters &parameters) = 0;
	
	///
	/// get tracked parameters of a node
	/// @param nodeId id of node
	/// @param parameters parameters to get
	/// @return true if node exists in the ZWave network
	virtual bool get(uint32_t nodeId, Parameters &parameters) = 0;
};
