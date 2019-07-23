#include <iostream>
#include "FibaroFgr222.hpp"


FibaroFgr222::~FibaroFgr222() {
}

void FibaroFgr222::sendSet(Sender & sender, Parameters const & parameters) {
	uint8_t flags = 0;
	optional<uint8_t> blinds = parameters.getPercentage("position.blinds");
	optional<uint8_t> slat = parameters.getPercentage("position.slat");
	
	if (blinds) {
		flags |= 2;
		
		// workaround: slat does not work if blinds is zero and current values of blinds and slat are zero 
		if (*blinds == 0 && slat && *slat > 0)
			blinds = 1;
	}
	if (slat)
		flags |= 1;

	if (flags) {
		// byte sequence from Fibaro_FGRM222 handler in FHEM/10_ZWave.pm
		uint8_t const setPosition[] = {MANUFACTURER_PROPRIETARY, 0x01, 0x0f, 0x26, 0x01, flags, *blinds, *slat};
		sender.send(setPosition);
	}
}

void FibaroFgr222::sendGet(Sender & sender) {
	// get state of blinds and slat
	// response: 04 00 nodeId 08 91 01 0f 26 03 flags blinds slat
	uint8_t const getPosition[] {MANUFACTURER_PROPRIETARY, 0x01, 0x0f, 0x26, 0x02, 0x02, 0x00, 0x00};
	sender.send(getPosition);
}

void FibaroFgr222::get(Parameters & parameters) {
	parameters.setByte("position.blinds", this->blinds);
	parameters.setByte("position.slat", this->slat);
}

void FibaroFgr222::onCommand(ZWaveNetwork::Node & node, uint8_t const * data, int length, Sender & sender) {
	// data = MANUFACTURER_PROPRIETARY 01 0f 26 03 flags blinds slat
	if (length >= 8) {
		uint8_t flags = data[5];
		if (flags & 2)
			this->blinds = data[6];
		if (flags & 1)
			this->slat = data[7];
		#ifdef DEBUG_NETWORK
		std::cout << "Node " << node.toString() << ": blinds=" << int(this->blinds) << " slat=" << int(this->slat) << std::endl;
		#endif
	}
}


// FibaroFgr222Config

FibaroFgr222Config::~FibaroFgr222Config() {
}

void FibaroFgr222Config::sendSet(Sender & sender, Parameters const & parameters) {
	ConfigCommand::sendSet(sender, parameters);
	
	if (optional<uint16_t> slatTime = parameters.getWord("config.slatTime")) {
		// device does not report the new value, therefore set it directly
		this->slatTime = *slatTime;
		sendWord(sender, SLAT_TIME, *slatTime);
	} else if (parameters.contains("config.calibrate")) {
		sendByte(sender, CALIBRATE, 1);
	}
}

void FibaroFgr222Config::sendGet(Sender & sender) {
	uint8_t const getSlatTime[] {CONFIGURATION, GET, SLAT_TIME};
	sender.send(getSlatTime);
}

void FibaroFgr222Config::get(Parameters & parameters) {
	parameters.setWord("config.slatTime", this->slatTime);
}

void FibaroFgr222Config::onByte(ZWaveNetwork::Node & node, uint8_t index, uint8_t value, Sender & sender) {
}

void FibaroFgr222Config::onWord(ZWaveNetwork::Node & node, uint8_t index, uint16_t value, Sender & sender) {
	switch (index) {
	case SLAT_TIME:
		this->slatTime = value;
		#ifdef DEBUG_NETWORK
		std::cout << "Node " << node.toString() << ": slatTime=" << int(value) << std::endl;
		#endif
		break;
	}
}
