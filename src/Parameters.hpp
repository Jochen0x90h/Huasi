#pragma once

#include <string>
#include <map>
#include "optional.hpp"


///
/// list of parameters with convenience functions
struct Parameters {
public:

	bool contains(std::string const & name) const;
	
	optional<bool> getState(std::string const & name) const;
	void setState(std::string const & name, bool value);
	
	optional<uint8_t> getByte(std::string const & name) const;
	optional<uint8_t> getPercentage(std::string const & name) const;
	void setByte(std::string const & name, uint8_t value);

	optional<uint16_t> getWord(std::string const & name) const;
	void setWord(std::string const & name, uint16_t value);

	std::map<std::string, std::string> parameters;
};
