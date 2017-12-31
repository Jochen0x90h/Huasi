#include <iostream>
#include "HttpChannel.h"


// http error category
class HttpCategory : public error_category {
public:
	const char *name() const noexcept override {
		return "http";
	}
	
	std::string message(int val) const override {
		return http_errno_description(http_errno(val));
	}
};
static HttpCategory httpCategory;
error_category & getHttpCategory() {
	return httpCategory;
}

// Response

HttpChannel::Response::Response(int status, std::string const & message) {
	this->s << "HTTP/1.1 " << status << ' ' << message << "\r\n";
}

void HttpChannel::Response::addHeader(std::string const & key, std::string const & value) {
	this->s << key << ": " << value << "\r\n";
}

void HttpChannel::Response::addHeaders(Headers const & headers) {
	for (auto p : headers) {
		this->s << p.first << ": " << p.second << "\r\n";
	}
}

void HttpChannel::Response::addClose() {
	this->s << "Connection: close\r\n";
}

void HttpChannel::Response::addContent(std::string const & contentType, size_t contentLength) {
	this->s << "Content-Type: " << contentType << "\r\n";
	this->s << "Content-Length: " << contentLength << "\r\n";
}

// HttpChannel

HttpChannel::HttpChannel(asio::io_service & loop, int timeout)
		: Channel(loop, timeout) {
	this->parser.data = this;
}

HttpChannel::~HttpChannel() {
}

void HttpChannel::sendResponse(Response const & response) {
	std::string data = response.s.str();

	// end of headers
	data += "\r\n";
	sendData(data);
}

void HttpChannel::onConnect() {
	// init for http server
	http_parser_init(&this->parser, HTTP_REQUEST);
}

void HttpChannel::onData(uint8_t const * data, size_t length) {
	size_t numParsed = http_parser_execute(&this->parser, &HttpChannel::callbacks, (char const *)data, length);
	if (numParsed != length) {
		// error
		http_errno error = HTTP_PARSER_ERRNO(&this->parser);
		onError(error_code(int(error), httpCategory));
	}
}

char const * HttpChannel::getMethodString(Method method) {
	#define XX(num, name, string) case Method::name: return #name;
	switch (method) {
	HTTP_METHOD_MAP(XX)
	}
	#undef XX
	return nullptr;
}

int HttpChannel::on_message_begin(http_parser *parser) {
	//std::cout << "on_message_begin " << std::endl;
	return 0;
}

int HttpChannel::on_url(http_parser *parser, const char *data, size_t length) {
	//std::cout << "on_url " << std::string(data, length) << std::endl;
	
	// server only
	HttpChannel *channel = (HttpChannel*)parser->data;
	channel->urlOrStatus.append(data, length);
	return 0;
}

int HttpChannel::on_status(http_parser *parser, const char *data, size_t length) {
	//std::cout << "on_status " << std::string(data, length) << std::endl;
	
	// client only
	//HttpChannel *channel = (HttpChannel*)parser->data;
	//channel->urlOrStatus.append(data, length);
	return 0;
}

int HttpChannel::on_header_field(http_parser *parser, const char *data, size_t length) {
	//std::cout << "on_header_field " << std::string(data, length) << std::endl;
	HttpChannel *channel = (HttpChannel*)parser->data;
	channel->moveHeader();
	channel->field.append(data, length);
	return 0;
}

int HttpChannel::on_header_value(http_parser *parser, const char *data, size_t length) {
	//std::cout << "on_header_value " << std::string(data, length) << std::endl;
	HttpChannel *channel = (HttpChannel*)parser->data;
	channel->valueValid = true;
	channel->value.append(data, length);
	return 0;
}

int HttpChannel::on_headers_complete(http_parser *parser) {
	//std::cout << "on_headers_complete " << std::endl;
	HttpChannel *channel = (HttpChannel*)parser->data;
	channel->moveHeader();
	channel->onRequest(Method(parser->method), std::move(channel->urlOrStatus), std::move(channel->headers));
	return 0;
}

int HttpChannel::on_body(http_parser *parser, char const * data, size_t length) {
	//std::cout << "on_body " << std::string(data, length) << std::endl;
	HttpChannel *channel = (HttpChannel*)parser->data;
	channel->onBody((uint8_t const *)data, length);
	return 0;
}

int HttpChannel::on_message_complete(http_parser *parser) {
	//std::cout << "on_message_complete " << std::endl;
	HttpChannel *channel = (HttpChannel*)parser->data;
	channel->onEnd();
	return 0;
}

int HttpChannel::on_chunk_header(http_parser *parser) {
	//std::cout << "on_chunk_header " << std::endl;
	return 0;
}

int HttpChannel::on_chunk_complete(http_parser *parser) {
	//std::cout << "on_chunk_complete " << std::endl;
	return 0;
}

const http_parser_settings HttpChannel::callbacks = {
	on_message_begin,
	on_url,
	on_status,
	on_header_field,
	on_header_value,
	on_headers_complete,
	on_body,
	on_message_complete,
	on_chunk_header,
	on_chunk_complete
};
