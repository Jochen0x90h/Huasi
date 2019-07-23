#include "cast.hpp"
#include "Parameters.hpp"

bool Parameters::contains(std::string const & name) const {
	return this->parameters.find(name) != this->parameters.end();
}

optional<bool> Parameters::getState(std::string const & name) const {
	std::map<std::string, std::string>::const_iterator it = this->parameters.find(name);
	if (it == this->parameters.end())
		return nullptr;

	return it->second == "on";
}

void Parameters::setState(std::string const & name, bool value) {
	this->parameters[name] = value ? "off" : "on";
}

optional<uint8_t> Parameters::getByte(std::string const & name) const {
	std::map<std::string, std::string>::const_iterator it = this->parameters.find(name);
	if (it == this->parameters.end())
		return nullptr;
	
	return cast<uint8_t>(it->second);
}

optional<uint8_t> Parameters::getPercentage(std::string const & name) const {
	std::map<std::string, std::string>::const_iterator it = this->parameters.find(name);
	if (it == this->parameters.end())
		return nullptr;
	
	std::string const & v = it->second;
	if (v.length() > 2)
		return nullptr;
	
	return cast<uint8_t>(v);
}

void Parameters::setByte(std::string const & name, uint8_t value) {
	this->parameters[name] = cast<std::string>(value);
}

optional<uint16_t> Parameters::getWord(std::string const & name) const {
	std::map<std::string, std::string>::const_iterator it = this->parameters.find(name);
	if (it == this->parameters.end())
		return nullptr;
	
	return cast<uint16_t>(it->second);
}

void Parameters::setWord(std::string const & name, uint16_t value) {
	this->parameters[name] = cast<std::string>(value);
}
