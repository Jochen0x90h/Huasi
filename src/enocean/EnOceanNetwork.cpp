#include <iostream>
#include <iomanip>
#include "EnOceanNetwork.hpp"


// CommonCommandRequest

EnOceanNetwork::CommonCommandRequest::~CommonCommandRequest() {
}

int EnOceanNetwork::CommonCommandRequest::getData(uint8_t *data) {
	data[0] = this->code;
	return 1;
}

int EnOceanNetwork::CommonCommandRequest::getOptionalData(uint8_t *data) {
	return 0;
}

void EnOceanNetwork::CommonCommandRequest::onResponse(EnOceanProtocol *protocol, const uint8_t *data, int length,
	const uint8_t *optionalData, int optionalLength)
{
	uint8_t result = data[0];
	switch (this->code) {
	case RD_IDBASE:
		if (length >= 5) {
			uint32_t baseId = (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];
			std::cout << "baseId " << std::hex << baseId << std::dec << std::endl;
		}
		if (optionalLength >= 1) {
			int remainingWrites = optionalData[0];
			std::cout << "remainingWrites " << remainingWrites << std::endl;
		}
		break;
	}
}


// EnOceanNetwork

EnOceanNetwork::EnOceanNetwork(asio::io_service &service, const std::string &device)
	: EnOceanProtocol(service, device)
{
	// get list of nodes in the network
	sendRequest(new CommonCommandRequest(CommonCommandRequest::RD_IDBASE));
}

EnOceanNetwork::~EnOceanNetwork() {
}

bool EnOceanNetwork::sendSet(uint32_t nodeId, Parameters const & parameters) {
	/*if (nodeId >= 0 && nodeId < 256) {
		Node & node = this->nodes[nodeId];
		if (!node.commands.empty()) {
			Command::Sender sender(this, nodeId);
			for (std::pair<uint8_t, ptr<Command>> p : node.commands) {
				p.second->sendSet(sender, parameters);
			}
			return true;
		}
	}*/
	return false;
}

bool EnOceanNetwork::get(uint32_t nodeId, Parameters & parameters) {
	/*if (nodeId >= 0 && nodeId < 256) {
		Node & node = this->nodes[nodeId];
		parameters.parameters["node.name"] = node.name;
		if (!node.deviceName.empty())
			parameters.parameters["device.name"] = node.deviceName;

		if (!node.commands.empty()) {
			for (std::pair<uint8_t, ptr<Command>> p : node.commands) {
				p.second->get(parameters);
			}
			return true;
		}
	}*/
	return false;
}

void EnOceanNetwork::onRequest(uint8_t packetType, const uint8_t *data, int length,
	const uint8_t *optionalData, int optionalLength)
{
	if (packetType == RADIO_ERP1) {
		if (length >= 7 && data[0] == 0xf6) {
			
			uint32_t nodeId = (data[2] << 24) | (data[3] << 16) | (data[4] << 8) | data[5];
			int state = data[1];
			std::cout << "nodeId " << std::hex << nodeId << " state " << state << std::dec << std::endl;
		}
		if (optionalLength >= 7) {
			uint32_t destinationId = (optionalData[1] << 24) | (optionalData[2] << 16) | (optionalData[3] << 8) | optionalData[4];
			int dBm = -optionalData[5];
			int security = optionalData[6];
			std::cout << "destinationId " << std::hex << destinationId << std::dec << " dBm " << dBm << " security " << security << std::endl;
		}
	}
}
