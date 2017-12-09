#pragma once

#include <string>
#include "http-parser/http_parser.h"


class Url {
public:
	Url(std::string url) : url(std::move(url)) {
		http_parser_url_init(&parsedUrl);
		http_parser_parse_url(this->url.data(), this->url.length(), false, &parsedUrl);
	}
	Url(const char *url) : url(url) {
		http_parser_url_init(&parsedUrl);
		http_parser_parse_url(this->url.data(), this->url.length(), false, &parsedUrl);
	}
	
	bool hasSchema() const {return parsedUrl.field_set & (1 << UF_SCHEMA);}
	bool hasHost() const {return parsedUrl.field_set & (1 << UF_HOST);}
	bool hasPort() const {return parsedUrl.field_set & (1 << UF_PORT);}
	bool hasPath() const {return parsedUrl.field_set & (1 << UF_PATH);}
	bool hasQuery() const {return parsedUrl.field_set & (1 << UF_QUERY);}
	bool hasFragment() const {return parsedUrl.field_set & (1 << UF_FRAGMENT);}
	bool hasUserinfo() const {return parsedUrl.field_set & (1 << UF_USERINFO);}
	
	std::string getSchema() const {
		auto f = this->parsedUrl.field_data[UF_SCHEMA];
		return this->url.substr(f.off, f.len);
	}

	std::string getHost() const {
		auto f = this->parsedUrl.field_data[UF_HOST];
		return this->url.substr(f.off, f.len);
	}

	int getPort() const {
		return this->parsedUrl.port;
	}

	std::string getPath() const {
		auto f = this->parsedUrl.field_data[UF_PATH];
		return this->url.substr(f.off, f.len);
	}
	
	///
	/// get path and query for use in a http request (origin-form from https://tools.ietf.org/html/rfc7230#section-5.3)
	/// e.g. /foo?bar=1
	std::string getOrigin() const {
		auto f = this->parsedUrl.field_data[UF_PATH];
		auto g = this->parsedUrl.field_data[UF_QUERY];
		int len = f.len + g.len + (uint16_t(-g.len) >> 15);
		return this->url.substr(f.off, len);
	}

	std::string getQuery() const {
		auto f = this->parsedUrl.field_data[UF_QUERY];
		return this->url.substr(f.off, f.len);
	}

	std::string getFragment() const {
		auto f = this->parsedUrl.field_data[UF_FRAGMENT];
		return this->url.substr(f.off, f.len);
	}

	std::string getUserinfo() const {
		auto f = this->parsedUrl.field_data[UF_USERINFO];
		return this->url.substr(f.off, f.len);
	}

	const std::string &getUrl() const {
		return this->url;
	}

protected:
	std::string url;
	http_parser_url parsedUrl;
};
