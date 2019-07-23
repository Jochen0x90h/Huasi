#include <iostream>
#include <iomanip>
#include "ZWaveNetwork.hpp"
#include "FibaroFgr222.hpp"


// DiscoverNodesRequest

ZWaveNetwork::DiscoverNodesRequest::~DiscoverNodesRequest() {
}

int ZWaveNetwork::DiscoverNodesRequest::getRequest(uint8_t * data) {
	return 0;
}

void ZWaveNetwork::DiscoverNodesRequest::onResponse(ZWaveProtocol * protocol, uint8_t const * data, int length) {
	if (length >= 4) {
		//uint8_t version = data[0];
		//uint8_t capabilities = data[1];
		int byteCount = data[2];
		for (int byteIndex = 0; byteIndex < byteCount; ++byteIndex) {
			uint8_t nodeId = byteIndex * 8;
			for (uint8_t b = data[3 + byteIndex]; b != 0; b >>= 1) {
				++nodeId;
				if ((b & 1) != 0) {
					// node exists
					//std::cout << "found node " << int(nodeId) << std::endl;
					if (nodeId != 1)
						protocol->sendRequest(new GetNodeInfoRequest(nodeId));
				}
			}
		}
	}
}


// GetNodeInfoRequest

ZWaveNetwork::GetNodeInfoRequest::~GetNodeInfoRequest() {
}

int ZWaveNetwork::GetNodeInfoRequest::getRequest(uint8_t * data) {
	data[0] = this->nodeId;
	return 1;
}

void ZWaveNetwork::GetNodeInfoRequest::onResponse(ZWaveProtocol * protocol, uint8_t const * data, int length) {
/*
	if (length >= 5) {
		// received info for a node
		//std::cout << "type " << int(data[5]) << std::endl;
		ZWaveNetwork * network = (ZWaveNetwork *)protocol;
		Node & node = network->nodes[this->nodeId];
		switch (data[4]) {
		case Node::CONTROLLER:
		case Node::STATIC_CONTROLLER:
			std::cout << "Node " << int(this->nodeId) << ": Controller" << std::endl;
			break;
		case Node::SWITCH_BINARY:
			break;
		case Node::SWITCH_MULTILEVEL:
			{
				std::cout << "Node " << int(this->nodeId) << ": Multilevel Switch" << std::endl;
				//node.commands[Command::BASIC] = new BasicCommand();
				node.commands[Command::CONFIGURATION] = new FibaroFgr222Config();
				node.commands[Command::MANUFACTURER_PROPRIETARY] = new FibaroFgr222();
			}
			break;
		}
		
		// get current state
		Command::Sender sender(network, this->nodeId);
		for (std::pair<uint8_t, ptr<Command>> p : node.commands) {
			p.second->sendGet(sender);
		}
	}
*/
}


// SendDataRequest

ZWaveNetwork::SendDataRequest::~SendDataRequest() {
}

int ZWaveNetwork::SendDataRequest::getRequest(uint8_t * data) {
	// nodeId
	data[0] = this->nodeId;
	
	// length
	data[1] = uint8_t(this->data.size());
	
	// data
	std::copy(this->data.begin(), this->data.end(), data + 2);
	
	// TX_OPTIONS
	data[2 + this->data.size()] = SendDataRequest::ACK | SendDataRequest::AUTO_ROUTE;
	
	return 2 + this->data.size() + 1;
}

void ZWaveNetwork::SendDataRequest::onResponse(ZWaveProtocol * protocol, uint8_t const * data, int length) {
	if (length >= 1 && data[0] == SENT) {
		// data was sent
	} else {
		// something went wrong
		std::cout << "SendDataRequest::onResponse error sending data";
	}
}

void ZWaveNetwork::SendDataRequest::onRequest(ZWaveProtocol * protocol, uint8_t const * data, int length) {
	
}

/*
// SendCommandRequest

ZWaveNetwork::SendCommandRequest::~SendCommandRequest() {
}

void ZWaveNetwork::SendCommandRequest::add(uint8_t const * command, int length) {
	if (this->data.empty()) {
		this->data.assign(command, command + length);
	} else {
		if (this->data[0] != Command::MULTI_CMD) {
			const uint8_t multiCommand[] = {Command::MULTI_CMD, 0x01, 1, uint8_t(this->data.size())};
			this->data.insert(this->data.begin(), std::begin(multiCommand), std::end(multiCommand));
		}
		this->data.push_back(uint8_t(length));
		this->data.insert(this->data.end(), command, command + length);
		
		// increment number of commands in multi command
		++this->data[2];
	}
}
*/

// Command

ZWaveNetwork::Command::~Command() {
}


// BasicCommand

ZWaveNetwork::BasicCommand::~BasicCommand() {
}

void ZWaveNetwork::BasicCommand::sendSet(Sender & sender, Parameters const & parameters) {
	optional<bool> state = parameters.getState("state");
	optional<uint8_t> dim = parameters.getPercentage("dim");
	if (state || dim) {
		uint8_t value = dim ? *dim : (*state ? 0xff : 0x00);
		uint8_t const setValue[] = {BASIC, SET, value};
		sender.send(setValue);
	}
}

void ZWaveNetwork::BasicCommand::sendGet(Sender & sender) {
	uint8_t const getValue[] {BASIC, GET};
	sender.send(getValue);
}

void ZWaveNetwork::BasicCommand::get(Parameters & parameters) {
	if (this->value == 0x00 || this->value == 0xff)
		parameters.setState("state", this->value != 0);
	else
		parameters.setByte("dim", this->value);
}

void ZWaveNetwork::BasicCommand::onCommand(Node & node, uint8_t const * data, int length, Sender & sender) {
	// data = BASIC REPORT value
	if (length >= 3 && data[1] == REPORT) {
		this->value = data[2];
		std::cout << "BasicCommand::onCommand value: " << int(this->value) << std::endl;
	}
}


// ConfigCommand

ZWaveNetwork::ConfigCommand::~ConfigCommand() {
}

void ZWaveNetwork::ConfigCommand::sendSet(Sender & sender, Parameters const & parameters) {

}

void ZWaveNetwork::ConfigCommand::sendGet(Sender & sender) {
}

void ZWaveNetwork::ConfigCommand::get(Parameters & parameters) {
}

void ZWaveNetwork::ConfigCommand::sendByte(Sender & sender, uint8_t index, uint8_t value) {
	uint8_t const setValue[] = {CONFIGURATION, SET, index, 0x01, value};
	sender.send(setValue);
}

void ZWaveNetwork::ConfigCommand::sendWord(Sender & sender, uint8_t index, uint16_t value) {
	uint8_t const setValue[] = {CONFIGURATION, SET, index, 0x02,
			uint8_t(value >> 8),
			uint8_t(value)};
	sender.send(setValue);
}

void ZWaveNetwork::ConfigCommand::sendLong(Sender & sender, uint8_t index, uint32_t value) {
	uint8_t const setValue[] = {CONFIGURATION, SET, index, 0x04,
			uint8_t(value >> 24),
			uint8_t(value >> 16),
			uint8_t(value >> 8),
			uint8_t(value)};
	sender.send(setValue);
}

void ZWaveNetwork::ConfigCommand::onCommand(Node & node, uint8_t const * data, int length, Sender & sender) {
	// data = CONFIGURATION REPORT index size value
	if (length >= 5 && data[1] == REPORT) {
		uint8_t index = data[2];
		uint8_t size = data[3];
		if (length >= 4 + size) {
			switch (size) {
			case 1:
				onByte(node, index, data[4], sender);
				break;
			case 2:
				onWord(node, index, (data[4] << 8) | data[5], sender);
				break;
			case 4:
				onLong(node, index, (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7], sender);
				break;
			}
		}
	}
}

void ZWaveNetwork::ConfigCommand::onByte(Node & node, uint8_t index, uint8_t value, Sender & sender) {
}

void ZWaveNetwork::ConfigCommand::onWord(Node & node, uint8_t index, uint16_t value, Sender & sender) {
}

void ZWaveNetwork::ConfigCommand::onLong(Node & node, uint8_t index, uint32_t value, Sender & sender) {
}


// ManufacturerSpecificCommand

ZWaveNetwork::ManufacturerSpecificCommand::~ManufacturerSpecificCommand() {
}

void ZWaveNetwork::ManufacturerSpecificCommand::sendSet(Sender & sender, Parameters const & parameters) {
}

void ZWaveNetwork::ManufacturerSpecificCommand::sendGet(Sender & sender) {
	uint8_t const getModel[] {MANUFACTURER_SPECIFIC, GET};
	sender.send(getModel);
}

void ZWaveNetwork::ManufacturerSpecificCommand::get(Parameters & parameters) {
	parameters.setWord("device.manufacturer", this->manufacturer);
	parameters.setWord("device.product", this->product);
	parameters.setWord("device.id", this->id);
}

void ZWaveNetwork::ManufacturerSpecificCommand::onCommand(Node & node, uint8_t const * data, int length,
		Sender & sender) {
	// data = MANUFACTURER_SPECIFIC REPORT manufacturer[2] product[2] id[2]
	if (length >= 8) {
		this->manufacturer = (data[2] << 8) | data[3];
		this->product = (data[4] << 8) | data[5];
		this->id = (data[6] << 8) | data[7];
		
		std::map<Class, ptr<ZWaveNetwork::Command>> commands;
		
		// check for specific devices
		if (this->manufacturer == 271 && this->product == 770) {
			// Fibaro FGR-222
			node.deviceName = "Fibaro FGR-222";
			commands[CONFIGURATION] = new FibaroFgr222Config();
			commands[MANUFACTURER_PROPRIETARY] = new FibaroFgr222();
		}
		#ifdef DEBUG_NETWORK
		if (!node.deviceName.empty())
			std::cout << "Node " << node.name << ": " << node.deviceName << std::endl;
		#endif

		// get current state for new commands
		for (std::pair<Class, ptr<ZWaveNetwork::Command>> p : commands) {
			node.commands[p.first] = p.second;
			p.second->sendGet(sender);
		}
	}
}


// ZWaveNetwork

ZWaveNetwork::ZWaveNetwork(asio::io_service & service, std::string const & device)
		: ZWaveProtocol(service, device) {
	// get list of nodes in the network
	sendRequest(new DiscoverNodesRequest());
}

ZWaveNetwork::~ZWaveNetwork() {
}

bool ZWaveNetwork::sendSet(uint32_t nodeId, Parameters const & parameters) {
	if (nodeId >= 0 && nodeId < 256) {
		Node & node = this->nodes[nodeId];
		if (!node.commands.empty()) {
			Command::Sender sender(this, nodeId);
			for (std::pair<uint8_t, ptr<Command>> p : node.commands) {
				p.second->sendSet(sender, parameters);
			}
			return true;
		}
	}
	return false;
}

bool ZWaveNetwork::get(uint32_t nodeId, Parameters & parameters) {
	if (nodeId >= 0 && nodeId < 256) {
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
	}
	return false;
}

void ZWaveNetwork::onRequest(uint8_t const * data, int length) {
	// check if at least result and nodeId are present
	if (length >= 3) {
		uint8_t function = data[0];
		//uint8_t rxStatus = data[1];
		uint8_t nodeId = data[2];
	
		if (function == APPLICATION_COMMAND_HANDLER && length >= 7) {
			uint8_t commandLength = data[3];
			Command::Class commandClass = (Command::Class)data[4];
			if (4 + commandLength <= length) {
				Node & node = this->nodes[nodeId];
				
				std::map<Command::Class, ptr<Command>>::const_iterator it = node.commands.find(commandClass);
				if (it != node.commands.end()) {
					Command::Sender sender(this, nodeId);
					it->second->onCommand(node, data + 4, commandLength, sender);
				}
			}
		} else if (function == ZW_APPLICATION_UPDATE) {
			std::cout << "ZWaveNetwork::onRequest APPLICATION_UPDATE" << std::endl;
			if (length >= 7) {
				uint8_t generic = data[5];
				int end;
				for (end = 7; end < length; ++end) {
					if (data[end] == Command::MARK)
						break;
				}
				updateNode(nodeId, generic, data, end - 7);
			}
		}
	}
}

void ZWaveNetwork::updateNode(uint8_t nodeId, uint8_t generic, uint8_t const * classes, int classCount) {
	Node & node = this->nodes[nodeId];
	node.name = cast<std::string>(nodeId);

	std::cout << "Node " << node.name << ": ";
	switch (generic) {
	case Node::CONTROLLER:
	case Node::STATIC_CONTROLLER:
		std::cout << "Controller" << std::endl;
		break;
	case Node::SWITCH_BINARY:
		break;
	case Node::SWITCH_MULTILEVEL:
		std::cout << "Multilevel Switch";
		break;
	default:
		std::cout << "Unknown";
	}
	std::cout << std::endl;

	for (int i = 0; i < classCount; ++i) {
		switch (classes[i]) {
		case Command::BASIC:
			node.commands[Command::BASIC] = new BasicCommand();
			break;
		case Command::CONFIGURATION:
			node.commands[Command::CONFIGURATION] = new ConfigCommand();
			break;
		case Command::MANUFACTURER_SPECIFIC:
			node.commands[Command::MANUFACTURER_SPECIFIC] = new ManufacturerSpecificCommand();
			break;
		}
	}

	// get current state
	Command::Sender sender(this, nodeId);
	for (std::pair<uint8_t, ptr<Command>> p : node.commands) {
		p.second->sendGet(sender);
	}
}
