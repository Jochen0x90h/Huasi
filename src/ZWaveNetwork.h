#pragma once

#include "ZWaveProtocol.h"
#include "Parameters.h"
#include "cast.h"


///
/// Class for setting and tracking the state of the nodes in a ZWave network
/// https://github.com/yepher/RaZBerry
class ZWaveNetwork : public ZWaveProtocol {
public:

	///
	/// Request for discovering all nodes in the network
	class DiscoverNodesRequest : public Request {
	public:
		DiscoverNodesRequest() : Request(SERIAL_API_GET_INIT_DATA) {}
		~DiscoverNodesRequest() override;
		int getRequest(uint8_t * data) override;
		void onResponse(ZWaveProtocol * protocol, uint8_t const * data, int length) override;
	};

	///
	/// Request for obtaining info for a node (e.g. device class and subclass)
	class GetNodeInfoRequest : public Request {
	public:
		GetNodeInfoRequest(uint8_t nodeId) : Request(ZW_REQUEST_NODE_INFO), nodeId(nodeId) {}
		~GetNodeInfoRequest() override;
		int getRequest(uint8_t * data) override;
		void onResponse(ZWaveProtocol * protocol, uint8_t const * data, int length) override;
	protected:
		uint8_t nodeId;
	};

	
	///
	/// Request for sending data to a node
	/// See section 5.3.3.1 ZW_SendData in "Z-Wave ZW0201/ZW0301 Appl. Prg. Guide v4.50"
	class SendDataRequest : public SendRequest {
	public:
		///
		/// Send options
		enum Option {
			// request transfer acknowledge from receiving node to ensure proper transmission
			ACK = 0x01,
			
			// low transmit power (if node is closer than 2 meters)
			LOW_POWER = 0x02,
			
			// try to transmit via repeater nodes if direct transmission is not possible
			AUTO_ROUTE = 0x04,
			
			// disable routing
			NO_ROUTE = 0x10,
			
			// support explorer frames for automatic fixing of the routing table
			EXPLORE = 0x20
		};
		
		enum State {
			SENT = 0x01
		};

		//SendDataRequest(uint8_t nodeId) : nodeId(nodeId) {
		//}
		template <typename T, int L>
		SendDataRequest(uint8_t nodeId, T (&data)[L])
			: SendRequest(ZW_SEND_DATA), nodeId(nodeId), data(data, data + L) {
		}
		~SendDataRequest() override;
		int getRequest(uint8_t * data) override;
		void onResponse(ZWaveProtocol * protocol, uint8_t const * data, int length) override;
		void onRequest(ZWaveProtocol * protocol, uint8_t const * data, int length) override;

	protected:
		uint8_t nodeId;
		std::vector<uint8_t> data;
	};


	///
	/// Request for sending one or multiple commands to a node
/*	class SendCommandRequest : public SendDataRequest {
	public:
		
		SendCommandRequest(uint8_t nodeId) : SendDataRequest(nodeId) {}
		~SendCommandRequest() override;
		
		template <typename T, int L>
		void add(T (&command)[L]) {
			add(command, L);
		}
		void add(uint8_t const * command, int length);
		
		bool isEmpty() {return this->data.empty();}
	};
*/
	struct Node;
	
	///
	/// Command class of a node (one node may have one or more command class objects)
	class Command : public Object {
		friend class ZWaveNetwork;
	public:
		enum Class {
			BASIC = 0x20,
			CONTROLLER_REPLICATION = 0x21,
			SWITCH_BINARY = 0x25,
			SWITCH_MULTILEVEL = 0x26,
			SWITCH_ALL = 0x27,
			SCENE_ACTIVATION = 0x2B,
			SCENE_ACTUATOR_CONF = 0x2C,
			SENSOR_BINARY = 0x30,
			SENSOR_MULTILEVEL = 0x31,
			METER = 0x32,
			CLIMATE_CONTROL_SCHEDULE = 0x46,
			MULTI_INSTANCE = 0x60,
			CONFIGURATION = 0x70,
			ALARM = 0x71,
			MANUFACTURER_SPECIFIC = 0x72,
			POWERLEVEL = 0x73,
			PROTECTION = 0x75,
			NODE_NAMING = 0x77,
			BATTERY	= 0x80,
			HAIL = 0x82,
			WAKE_UP = 0x84,
			MULTI_CMD = 0x8F, // "Z-Wave Transport-Encapsulation Command Class Specification" section 3.4.3
			CLOCK = 0x81,
			ASSOCIATION = 0x85,
			VERSION = 0x86,
			MULTI_CHANNEL_ASSOCIATION = 0x8e,
			MANUFACTURER_PROPRIETARY = 0x91,
			MARK = 0xEF,
		};

		class Sender {
		public:
			Sender(ZWaveProtocol * protocol, uint8_t nodeId) : protocol(protocol), nodeId(nodeId) {}
			
			template <typename T, int L>
			void send(T (&data)[L]) {
				this->protocol->sendRequest(new SendDataRequest(this->nodeId, data));
			}
		protected:
			ZWaveProtocol * protocol;
			uint8_t nodeId;
		};

		virtual ~Command();
		
		///
		/// Send set command(s) to set the parameter(s) of the given node
		virtual void sendSet(Sender & sender, Parameters const & parameters) = 0;

		///
		/// Send get command(s) to get the parameter(s) from the given node (onCommand will be called on arrival)
		virtual void sendGet(Sender & sender) = 0;
		
		///
		/// Get tracked parameters of node (stored in this object)
		virtual void get(Parameters & parameters) = 0;

	protected:

		///
		/// A command has arrived from the ZWave network (e.g. report current parameter value)
		virtual void onCommand(Node & node, uint8_t const * data, int length, Sender & sender) = 0;
	};

	///
	/// Basic command class (can be used to track the value only if the node reports it on state change)
	class BasicCommand : public Command {
	public:
		// commands of basic command class
		enum Command {
			SET = 0x01,
			GET = 0x02,
			REPORT = 0x03
		};

		BasicCommand() : value() {}
		~BasicCommand() override;
		void sendSet(Sender & sender, Parameters const & parameters) override;
		void sendGet(Sender & sender) override;
		void get(Parameters & parameters) override;

	protected:
		void onCommand(Node & node, uint8_t const * data, int length, Sender & sender) override;

		// tracked value of the node
		uint8_t value;
	};

	///
	/// Config command class
	class ConfigCommand : public Command {
	public:
		// commands of config command class
		enum Command {
			SET = 0x04,
			GET = 0x05,
			REPORT = 0x06
		};

		~ConfigCommand() override;
		
		void sendSet(Sender & sender, Parameters const & parameters) override;
		void sendGet(Sender & sender) override;
		void get(Parameters & parameters) override;

		void sendByte(Sender & sender, uint8_t index, uint8_t value);
		void sendWord(Sender & sender, uint8_t index, uint16_t value);
		void sendLong(Sender & sender, uint8_t index, uint32_t value);
	
	protected:
		void onCommand(Node & node, uint8_t const * data, int length, Sender & sender) override;

		virtual void onByte(Node & node, uint8_t index, uint8_t value, Sender & sender);
		virtual void onWord(Node & node, uint8_t index, uint16_t value, Sender & sender);
		virtual void onLong(Node & node, uint8_t index, uint32_t value, Sender & sender);
	};

	class ManufacturerSpecificCommand : public Command {
	public:
		// commands of manufacturer specific command class
		enum Command {
			GET = 0x04,
			REPORT = 0x05
		};

		ManufacturerSpecificCommand() : manufacturer(), product(), id() {}
		~ManufacturerSpecificCommand() override;
		void sendSet(Sender & sender, Parameters const & parameters) override;
		void sendGet(Sender & sender) override;
		void get(Parameters & parameters) override;

	protected:
		void onCommand(Node & node, uint8_t const * data, int length, Sender & sender) override;
	
		uint16_t manufacturer;
		uint16_t product;
		uint16_t id;
	};

	///
	/// Node in the ZWave network
	struct Node {
		enum Class {
			CONTROLLER = 0x01,
			STATIC_CONTROLLER = 0x02,
			THERMOSTAT = 0x08,
			SWITCH_BINARY = 0x10,
			SWITCH_MULTILEVEL = 0x11,
			SWITCH_TOGGLE = 0x13,
			SENSOR_BINARY = 0x20,
			SENSOR_MULTILEVEL = 0x21,
			SENSOR_ALARM = 0xA1,
		};

		// name of device (e.g. Fibaro FGR-222)
		std::string deviceName;
		
		// name of node, assigned externally (e.g. LivingRoomLight)
		std::string name;

		// command classes
		std::map<Command::Class, ptr<Command>> commands;

		#ifdef DEBUG_NETWORK
		inline std::string toString() {
			std::stringstream s;
			s << this->name;
			if (!this->deviceName.empty())
				s << " (" << this->deviceName << ")";
			return s.str();
		}
		#endif
	};


	///
	/// Constructor
	/// @param loop event loop for asynchronous io
	/// @param device serial device of the zwave dongle
	ZWaveNetwork(asio::io_service & service, std::string const & device);

	~ZWaveNetwork() override;

	///
	/// send parameters to a node
	/// @param nodeId id of node
	/// @param parameters parameters to set
	/// @return true if node exists in the ZWave network
	bool sendSet(int nodeId, Parameters const & parameters);
	
	///
	/// get tracked parameters of a node
	/// @param nodeId id of node
	/// @param parameters parameters to get
	/// @return true if node exists in the ZWave network
	bool get(int nodeId, Parameters & parameters);
	
protected:

	void onRequest(uint8_t const * data, int length) override;

	void updateNode(uint8_t nodeId, uint8_t generic, uint8_t const * classes, int classCount);
	

	Node nodes[256];
};
