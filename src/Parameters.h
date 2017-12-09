#pragma once

#include <string>
#include <map>


///
/// list of parameters with convenience functions
struct Parameters {
public:

	bool getState(std::string const & name, uint8_t & value) const;
	void setState(std::string const & name, uint8_t value);
	
	bool getPercentage(std::string const & name, uint8_t & value) const;
	void setPercentage(std::string const & name, uint8_t value);

	std::map<std::string, std::string> parameters;
};
