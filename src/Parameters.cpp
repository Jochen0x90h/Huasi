#include "Parameters.h"


bool Parameters::getState(std::string const & name, uint8_t & value) const {
	std::map<std::string, std::string>::const_iterator it = this->parameters.find(name);
	if (it == this->parameters.end())
		return false;

	value = it->second == "on" ? 0xff : 0x00;
	return true;
}

void Parameters::setState(std::string const & name, uint8_t value) {
	this->parameters[name] = value == 0 ? "off" : "on";
}

bool Parameters::getPercentage(std::string const & name, uint8_t & value) const {
	std::map<std::string, std::string>::const_iterator it = this->parameters.find(name);
	if (it == this->parameters.end())
		return false;
	
	std::string const & v = it->second;
	
	if (v.length() > 2)
		return false;
	
	for (char ch : v) {
		value *= 10;
		if (ch < '0' || ch > '9')
			return false;
		value += ch - '0';
	}
	return true;
}

void Parameters::setPercentage(std::string const & name, uint8_t value) {
	char buffer[4];
	char * s = buffer + 3;
	*s = 0;
	do {
		--s;
		*s = '0' + value % 10;
		value /= 10;
	} while (value > 0);
	this->parameters[name] = s;
}
