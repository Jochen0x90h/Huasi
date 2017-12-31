#pragma once

#include <string>
#include <map>
#include "opt.h"


///
/// list of parameters with convenience functions
struct Parameters {
public:

	bool contains(std::string const & name) const;
	
	opt<bool> getState(std::string const & name) const;
	void setState(std::string const & name, bool value);
	
	opt<uint8_t> getByte(std::string const & name) const;
	opt<uint8_t> getPercentage(std::string const & name) const;
	void setByte(std::string const & name, uint8_t value);

	opt<uint16_t> getWord(std::string const & name) const;
	void setWord(std::string const & name, uint16_t value);

	std::map<std::string, std::string> parameters;
};
