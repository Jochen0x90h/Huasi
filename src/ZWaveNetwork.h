#pragma once

#include "ZWaveProtocol.h"
#include "Parameters.h"


///
/// Class for setting and tracking the state of the nodes in a ZWave network
/// https://github.com/yepher/RaZBerry
class ZWaveNetwork : public ZWaveProtocol {
public:

	///
	/// Request for discovering all nodes in the network
	class DiscoverNodesRequest : public Request {
	public:
		DiscoverNodesRequest(ZWaveNetwork * network) : network(network) {}
		~DiscoverNodesRequest() override;
		int getRequest(uint8_t * data) noexcept override;
		void onResponse(uint8_t const * data, int length) noexcept override;
	protected:
		ZWaveNetwork * network;
	};

	///
	/// Request for obtaining info for a node (e.g. device class and subclass)
	class GetNodeInfoRequest : public Request {
	public:
		GetNodeInfoRequest(ZWaveNetwork * network, uint8_t nodeId) : network(network), nodeId(nodeId) {}
		~GetNodeInfoRequest() override;
		int getRequest(uint8_t * data) noexcept override;
		void onResponse(uint8_t const * data, int length) noexcept override;
	protected:
		ZWaveNetwork * network;
		uint8_t nodeId;
	};

	///
	/// Request for sending data to a node (e.g. set on/off)
	class SendDataRequest : public Request {
	public:
		template <typename T, int L>
		SendDataRequest(T (&data)[L]) : length(L) {
			std::copy(data, data + L, this->data);
		}
		~SendDataRequest() override;
		int getRequest(uint8_t * data) noexcept override;
		void onResponse(uint8_t const * data, int length) noexcept override;
	protected:
		uint8_t data[16];
		int length;
	};

	
	///
	/// State of a node (one node may have one or more state objects)
	class State : public Object {
	public:
		virtual ~State();
		virtual void onRequest(uint8_t const * data, int length) = 0;
		virtual void set(ZWaveNetwork * network, uint8_t nodeId, Parameters const & parameters) = 0;
		virtual void get(ZWaveNetwork * network, uint8_t nodeId) = 0;
		virtual void get(Parameters & parameters) = 0;
	};

	///
	/// State of a node supporting the basic command class (also reporting on state change)
	class BasicState : public State {
	public:
		virtual ~BasicState();
		void onRequest(uint8_t const * data, int length) override;
		void set(ZWaveNetwork * network, uint8_t nodeId, Parameters const & parameters) override;
		void get(ZWaveNetwork * network, uint8_t nodeId) override;
		void get(Parameters & parameters) override;

	protected:
		uint8_t value;
	};

	///
	/// State of Fibaro Roller Shutter 2
	/// note: Basic command class is supported but does not get reported on state change
	class FibaroFgrm222 : public State {
	public:
		virtual ~FibaroFgrm222();
		void onRequest(uint8_t const * data, int length) override;
		void set(ZWaveNetwork * network, uint8_t nodeId, Parameters const & parameters) override;
		void get(ZWaveNetwork * network, uint8_t nodeId) override;
		void get(Parameters & parameters) override;
	
	protected:
		uint8_t blinds;
		uint8_t slat;
	};

	struct Node {
		// command class -> state handler
		std::map<uint8_t, ptr<State>> states;
	};


	///
	/// Constructor
	/// @param loop event loop for asynchronous io
	/// @param device serial device of the zwave dongle
	ZWaveNetwork(asio::io_service & service, std::string const & device);

	~ZWaveNetwork() override;

	///
	/// set parameters to a node
	/// @param nodeId id of node
	/// @param parameters parameters to set
	/// @return true if node exists in the ZWave network
	bool set(int nodeId, Parameters const & parameters);
	
	///
	/// set parameters to a node
	/// @param nodeId id of node
	/// @param parameters parameters to get
	/// @return true if node exists in the ZWave network
	bool get(int nodeId, Parameters & parameters);
	
protected:

	void onRequest(uint8_t const * data, int length) noexcept override;

	enum Function {
		DISCOVER_NODES = 0x02,
		APPLICATION_COMMAND_HANDLER = 0x04,
		SEND_DATA = 0x13,
		GET_NODE_INFO = 0x41
	};
	
	enum DeviceType {
		CONTROLLER = 0x02,
		SWITCH = 0x10,
		DIMMER = 0x11
	};
	
	enum CommandClass {
		BASIC = 0x20,
		CONTROLLER_REPLICATION = 0x21,
		SWITCH_BINARY= 0x25,
		SWITCH_MULTILEVEL = 0x26,
		SWITCH_ALL = 0x27,
		SCENE_ACTIVATION = 0x2B,
		SCENE_ACTUATOR_CONF = 0x2C,
		SENSOR_BINARY = 0x30,
		SENSOR_MULTILEVEL = 0x31,
		CLIMATE_CONTROL_SCHEDULE = 0x46,
		MULTI_INSTANCE = 0x60,
		CONFIGURATION = 0x70,
		ALARM = 0x71,
		MANUFACTURER_SPECIFIC = 0x72,
		POWERLEVEL= 0x73,
		NODE_NAMING= 0x77,
		BATTERY	= 0x80,
		HAIL = 0x82,
		WAKE_UP = 0x84,
		MULTI_CMD = 0x8F,
		CLOCK = 0x81,
		ASSOCIATION = 0x85,
		VERSION = 0x86,
		MANUFACTURER_PROPRIETARY = 0x91,
		MARK = 0xEF,
	};
	
	// commands of basic command class
	enum Basic {
		BASIC_SET = 0x01,
		BASIC_GET = 0x02,
		BASIC_REPORT = 0x03
	};


	Node nodes[256];
};
