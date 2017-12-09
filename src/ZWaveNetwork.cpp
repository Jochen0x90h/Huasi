#include <iostream>
#include <iomanip>
#include "ZWaveNetwork.h"


// DiscoverNodesRequest

ZWaveNetwork::DiscoverNodesRequest::~DiscoverNodesRequest() {
}

int ZWaveNetwork::DiscoverNodesRequest::getRequest(uint8_t * data) noexcept {
	data[0] = DISCOVER_NODES;
	return 1;
}

void ZWaveNetwork::DiscoverNodesRequest::onResponse(uint8_t const * data, int length) noexcept {
	/*std::cout << "DiscoverNodesRequest::onResponse" << std::endl;
	for (int i = 0; i < length; ++i) {
		std::cout << std::setfill('0') << std::setw(2) << std::hex << int(data[i]) << std::dec << " ";
	}
	std::cout << std::endl;*/

	if (length >= 5 && data[0] == DISCOVER_NODES) {
		int byteCount = data[3];
		for (int byteIndex = 0; byteIndex < byteCount; ++byteIndex) {
			uint8_t nodeIndex = byteIndex * 8;
			for (uint8_t b = data[4 + byteIndex]; b != 0; b >>= 1) {
				++nodeIndex;
				if ((b & 1) != 0) {
					// node exists
					//std::cout << "found node " << int(nodeIndex) << std::endl;
					this->network->sendRequest(new GetNodeInfoRequest(this->network, nodeIndex));
				}
			}
		}
	}
}


// GetNodeInfoRequest

ZWaveNetwork::GetNodeInfoRequest::~GetNodeInfoRequest() {
}

int ZWaveNetwork::GetNodeInfoRequest::getRequest(uint8_t * data) noexcept {
	data[0] = GET_NODE_INFO;
	data[1] = this->nodeId;
	return 2;
}

void ZWaveNetwork::GetNodeInfoRequest::onResponse(uint8_t const * data, int length) noexcept {
	/*std::cout << "GetNodeInfoRequest::onResponse " << int(this->nodeId) << std::endl;
	for (int i = 0; i < length; ++i) {
		std::cout << std::setfill('0') << std::setw(2) << std::hex << int(data[i]) << std::dec << " ";
	}
	std::cout << std::endl;*/

	if (length >= 6 && data[0] == GET_NODE_INFO) {
		// received info for a node
		//std::cout << "type " << int(data[5]) << std::endl;
		Node & node = this->network->nodes[this->nodeId];
		switch (data[5]) {
		case CONTROLLER:
			std::cout << "Node " << int(this->nodeId) << ": Controller" << std::endl;
			break;
		case SWITCH:
			break;
		case DIMMER:
			{
				std::cout << "Node " << int(this->nodeId) << ": Dimmer" << std::endl;
				//node.states[BASIC] = new BasicState();
				node.states[MANUFACTURER_PROPRIETARY] = new FibaroFgrm222();
			}
			break;
		}
		
		// get current state
		for (std::pair<uint8_t, ptr<State>> p : node.states) {
			p.second->get(this->network, this->nodeId);
		}
	}
}


// SendDataRequest

ZWaveNetwork::SendDataRequest::~SendDataRequest() {
}

int ZWaveNetwork::SendDataRequest::getRequest(uint8_t * data) noexcept {
	data[0] = SEND_DATA;
	std::copy(this->data, this->data + this->length, data + 1);
	return 1 + this->length;
}

void ZWaveNetwork::SendDataRequest::onResponse(uint8_t const * data, int length) noexcept {
	/*std::cout << "SendDataRequest::onResponse" << std::endl;
	for (int i = 0; i < length; ++i) {
		std::cout << std::setfill('0') << std::setw(2) << std::hex << int(data[i]) << std::dec << " ";
	}
	std::cout << std::endl;*/
}


// State

ZWaveNetwork::State::~State() {
}


// BasicState

ZWaveNetwork::BasicState::~BasicState() {
}

void ZWaveNetwork::BasicState::onRequest(uint8_t const * data, int length) {
	if (length >= 2) {
		this->value = data[1];
		std::cout << "Basic value: " << int(this->value) << std::endl;
	}
}

void ZWaveNetwork::BasicState::set(ZWaveNetwork * network, uint8_t nodeId, Parameters const & parameters) {
	if (parameters.getState("state", value) || parameters.getPercentage("dim", value)) {
		uint8_t const setValue[] = {nodeId, 0x03, BASIC, BASIC_SET, value};
		network->sendRequest(new SendDataRequest(setValue));
	}
}

void ZWaveNetwork::BasicState::get(ZWaveNetwork * network, uint8_t nodeId) {
	// get state
	// response request: 04 00 nodeId 02 03 value
	uint8_t const getValue[] {nodeId, 0x02, BASIC, BASIC_GET};
	network->sendRequest(new SendDataRequest(getValue));
}

void ZWaveNetwork::BasicState::get(Parameters & parameters) {
	if (this->value == 0x00 || this->value == 0xff)
		parameters.setState("state", this->value);
	else
		parameters.setPercentage("dim", this->value);
}


// FibaroFgrm222

ZWaveNetwork::FibaroFgrm222::~FibaroFgrm222() {
}

void ZWaveNetwork::FibaroFgrm222::onRequest(uint8_t const * data, int length) {
	if (length >= 8) {
		uint8_t flags = data[4];
		if (flags & 2)
			this->blinds = data[5];
		if (flags & 1)
			this->slat = data[6];
		std::cout << "FibaroFgrm222::onRequest blinds: " << int(this->blinds) << " slat: " << int(this->slat) << std::endl;
	}
}

void ZWaveNetwork::FibaroFgrm222::set(ZWaveNetwork * network, uint8_t nodeId, Parameters const & parameters) {
	uint8_t flags = 0;
	uint8_t blinds = 0;
	uint8_t slat = 0;
	if (parameters.getPercentage("position.blinds", blinds)) {
		flags |= 2;
	}
	if (parameters.getPercentage("position.slat", slat)) {
		flags |= 1;
	}
	
	if (flags) {
		// byte sequence from Fibaro_FGRM222 handler in FHEM/10_ZWave.pm
		uint8_t const setPosition[] = {nodeId, 0x08, MANUFACTURER_PROPRIETARY, 0x01, 0x0f, 0x26, 0x01, flags, blinds, slat};
		network->sendRequest(new SendDataRequest(setPosition));
	}
}

void ZWaveNetwork::FibaroFgrm222::get(ZWaveNetwork * network, uint8_t nodeId) {
	// get state of blinds and slat
	// response: 04 00 nodeId 08 91 01 0f 26 03 flags blinds slat
	uint8_t const getPosition[] {nodeId, 0x08, MANUFACTURER_PROPRIETARY, 0x01, 0x0f, 0x26, 0x02, 0x02, 0x00, 0x00};
	network->sendRequest(new SendDataRequest(getPosition));
}

void ZWaveNetwork::FibaroFgrm222::get(Parameters & parameters) {
	parameters.setPercentage("position.blinds", this->blinds);
	parameters.setPercentage("position.slat", this->slat);
}


// ZWaveNetwork

ZWaveNetwork::ZWaveNetwork(asio::io_service & service, std::string const & device)
		: ZWaveProtocol(service, device) {

	sendRequest(new DiscoverNodesRequest(this));
}

ZWaveNetwork::~ZWaveNetwork() {
}

bool ZWaveNetwork::set(int nodeId, Parameters const & parameters) {
	if (nodeId >= 0 && nodeId < 256) {
		Node & node = this->nodes[nodeId];
		if (!node.states.empty()) {
			for (std::pair<uint8_t, ptr<State>> p : node.states) {
				p.second->set(this, nodeId, parameters);
			}
			return true;
		}
	}
	return false;
}

bool ZWaveNetwork::get(int nodeId, Parameters & parameters) {
	if (nodeId >= 0 && nodeId < 256) {
		Node & node = this->nodes[nodeId];
		if (!node.states.empty()) {
			for (std::pair<uint8_t, ptr<State>> p : node.states) {
				p.second->get(parameters);
			}
			return true;
		}
	}
	return false;
}

void ZWaveNetwork::onRequest(uint8_t const * data, int length) noexcept {
	/*std::cout << "ZWaveNetwork::onRequest" << std::endl;
	for (int i = 0; i < length; ++i) {
		std::cout << std::setfill('0') << std::setw(2) << std::hex << int(data[i]) << std::dec << " ";
	}
	std::cout << std::endl;*/
	
	if (length > 6 && data[0] == APPLICATION_COMMAND_HANDLER) {
		uint8_t nodeId = data[2];
		uint8_t commandLength = data[3];
		uint8_t commandClass = data[4];
		if (4 + commandLength <= length) {
			Node & node = this->nodes[nodeId];
			
			std::map<uint8_t, ptr<State>>::const_iterator it = node.states.find(commandClass);
			if (it != node.states.end()) {
				it->second->onRequest(data + 5, commandLength);
			}
		}
	}
}
