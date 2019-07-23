#pragma once

#include "ZWaveNetwork.hpp"


///
/// Fibaro Roller Shutter 2
/// note: Basic command class is supported but does not get reported on state change
class FibaroFgr222 : public ZWaveNetwork::Command {
public:
	FibaroFgr222() : blinds(), slat() {}
	~FibaroFgr222() override;
	void sendSet(Sender & sender, Parameters const & parameters) override;
	void sendGet(Sender & sender) override;
	void get(Parameters & parameters) override;

protected:
	void onCommand(ZWaveNetwork::Node & node, uint8_t const * data, int length, Sender & sender) override;

	// tracked values of the roller shutter
	uint8_t blinds;
	uint8_t slat;
};

///
/// Configuration of Fibaro Roller Shutter 2
/// https://wiki.fhem.de/wiki/Z-Wave-FIB_FGRM-222-Rollladenaktor
class FibaroFgr222Config : public ZWaveNetwork::ConfigCommand {
public:
	FibaroFgr222Config() : slatTime() {}
	~FibaroFgr222Config() override;
	void sendSet(Sender & sender, Parameters const & parameters) override;
	void sendGet(Sender & sender) override;
	void get(Parameters & parameters) override;

protected:

	enum Register {
		// slat turn time of venetian blind
		SLAT_TIME = 12,
		
		// calibrate blinds open/close time by writing an 1 to the register
		CALIBRATE = 29
	};

	void onByte(ZWaveNetwork::Node & node, uint8_t index, uint8_t value, Sender & sender) override;
	void onWord(ZWaveNetwork::Node & node, uint8_t index, uint16_t value, Sender & sender) override;

	// rotation time of slat in venetian blind mode
	uint16_t slatTime;
};
