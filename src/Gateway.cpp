#include <stdlib.h> // strtol
#include "Gateway.h"


namespace {
	
	// decode query string
	std::string decodeQuery(std::string const & s, size_t start, size_t end) {
		std::string r;
		for (size_t i = start; i < end; ++i) {
			char ch = s[i];
			if (ch == '+') {
				ch = ' ';
			} else if (ch == '%') {
				// check if two characters follow
				if (i + 2 >= end)
					break;
				
				// convert next two characters to hex
				ch = 0;
				for (int j = 1; j < 3; ++j) {
					ch <<= 4;
					char hex = s[i + j];
					if (hex >= '0' && hex <= '9')
						ch += hex - '0';
					else if (hex >= 'a' && hex <= 'f')
						ch += hex - 'a' + 10;
					else if (hex >= 'A' && hex <= 'F')
						ch += hex - 'A' + 10;
				}
				i += 2;
			}
			r += ch;
		}
		return r;
	}
	
	// encode query string
	std::string encodeQuery(std::string const & s) {
		std::string r;
		for (unsigned char ch : s) {
			if (ch == '-' || ch == '.' || ch == '_' || ch == '~'
					|| (ch >= 'A' && ch <= 'Z')
					|| (ch >= 'a' && ch <= 'z')
					|| (ch >= '0' && ch <= '9')) {
				// not reserved character
				r += ch;
			} else if (ch == ' ') {
				// space
				r += '+';
			} else {
				// hex escape
				static char const hex[] = "0123456789ABCDEF";
				r += '%';
				r += hex[ch >> 4];
				r += hex[ch & 15];
			}
		}
		return r;
	}
}

Gateway::~Gateway() {
}

void Gateway::onRequest(Method method, std::string url, Headers headers) {
	Url u(url);

	// parse path
	std::string path = u.getPath();
	
	std::string prefix("/node/");
	if (!path.compare(0, prefix.size(), prefix)) {
		int nodeId = atoi(path.data() + prefix.size());
	
		if (method == Method::POST) {
			// parse query
			Parameters parameters;
			std::string query = u.getQuery();
			size_t argStartPos = 0;
			while (argStartPos < query.length()) {
				// get an argument
				size_t argEndPos = query.find('&', argStartPos);
				if (argEndPos == std::string::npos)
					argEndPos = query.length();

				// split argument into key and value
				size_t eqPos = query.find('=', argStartPos);
				if (eqPos != std::string::npos && eqPos < argEndPos) {
					std::string key = query.substr(argStartPos, eqPos - argStartPos);
					std::string value = decodeQuery(query, eqPos + 1, argEndPos);

					parameters.parameters[key] = value;
				}
				
				argStartPos = argEndPos + 1;
			}
			
			// send parameters to node
			if (this->network->sendSet(nodeId, parameters)) {
				// send response
				Response response(200, "OK");
				response.addHeaders(Gateway::defaultHeaders);
				if (!isKeepAlive())
					response.addClose();
				sendResponse(response);
				return;
			}
		} else if (method == Method::GET) {
			// get tracked parameters from node
			Parameters parameters;
			if (this->network->get(nodeId, parameters)) {
				// build response body
				std::string data;
				for (std::pair<std::string, std::string> p : parameters.parameters) {
					if (!data.empty())
						data += '&';
					data += p.first;
					data += '=';
					data += encodeQuery(p.second);
				}
				
				// send response
				Response response(200, "OK");
				response.addHeaders(Gateway::defaultHeaders);
				if (!isKeepAlive())
					response.addClose();
				response.addContent("application/x-www-form-urlencoded", data.length());
				sendResponse(response);
				sendData(data);
				return;
			}
		}
	}

	Response response(404, "Not Found");
	response.addHeaders(Gateway::defaultHeaders);
	if (!isKeepAlive())
		response.addClose();
	sendResponse(response);
}

void Gateway::onBody(uint8_t const * data, size_t length) {
}

void Gateway::onEnd() {
	//close();
}

std::map<std::string, std::string> Gateway::defaultHeaders = {{"Server", "huasi"}};
