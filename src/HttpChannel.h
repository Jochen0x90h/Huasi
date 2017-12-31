#pragma once

#include <map>
#include "http-parser/http_parser.h"
#include "Url.h"
#include "Channel.h"


/// Get HTTP error category
error_category &getHttpCategory();

///
/// Communication channel for HTTP/1.1
class HttpChannel : public Channel {
public:
	// HTTP methods such as GET and POST
	enum class Method {
		#define XX(num, name, string) name = num,
		HTTP_METHOD_MAP(XX)
		#undef XX
	};
	
	using Headers = std::map<std::string, std::string>;

	///
	/// HTTP response containing status and headers
	class Response {
		friend class HttpChannel;
	public:
		Response(int status, std::string const & message);
		
		///
		/// Add a header
		void addHeader(std::string const & key, std::string const & value);

		///
		/// Add multiple headers
		void addHeaders(Headers const & headers);

		///
		/// Add Connection: close header
		void addClose();

		///
		/// Add Content-Type and Content-Length headers
		void addContent(std::string const & contentType, size_t contentLength);
	protected:
		std::ostringstream s;
	};
	
	///
	/// Constructor
	/// @param loop event loop for asynchronous io
	/// @param timeout inactivity timeout in milliseconds after which the channel is closed
	HttpChannel(asio::io_service & loop, int timeout);
	
	~HttpChannel() override;

	///
	/// Server mode: send a http response to the client
	/// @param response Response object dontaining the status and headers of the response
	void sendResponse(Response const & response);

	///
	/// Send (part of) http body
	void sendBody(uint8_t const * data, size_t length) {sendData(data, length);}

	///
	/// Send (part of) http body. use sendData(std::move(data)) to reduce copying of data
	void sendBody(std::string const & data) {sendData(data);}


	///
	/// Returns true if this is a keep alive connection. check in onRequest(), onResponse() or onEnd()
	/// Server mode: to close, respond with the "Connection: close" header
	/// Client mode: to close, close() the channel
	bool isKeepAlive() {return http_should_keep_alive(&this->parser) != 0;}

	///
	/// Get string representation of HTTP method such as GET and POST
	static char const * getMethodString(Method method);

protected:

	///
	/// Called when a client or server connection was established. Receiving is already enabled
	void onConnect() override;

	///
	/// Called when new data arrived
	void onData(uint8_t const * data, size_t length) override;

	///
	/// Server mode: gets called when a http request header arrived from the client
	/// @param method http method (e.g. GET)
	/// @param url url without protocol and host (e.g. "/foo/bar?foo=bar")
	/// @param http headers
	virtual void onRequest(Method method, std::string url, Headers headers) = 0;

	///
	/// A (part of) a http body was received
	virtual void onBody(uint8_t const * data, size_t length) = 0;

	///
	/// The end of a http message was received.
	virtual void onEnd() = 0;

	// http-parser callbacks
	static int on_message_begin(http_parser *parser);
	static int on_url(http_parser *parser, const char *data, size_t length);
	static int on_status(http_parser *parser, const char *data, size_t length);
	static int on_header_field(http_parser *parser, const char *data, size_t length);
	static int on_header_value(http_parser *parser, const char *data, size_t length);
	static int on_headers_complete(http_parser *parser);
	static int on_body(http_parser *parser, const char *data, size_t length);
	static int on_message_complete(http_parser *parser);
	static int on_chunk_header(http_parser *parser);
	static int on_chunk_complete(http_parser *parser);
	static const http_parser_settings callbacks;

	void moveHeader() {
		if (this->valueValid) {
			this->headers[std::move(this->field)] = std::move(this->value);
			this->valueValid = false;
		}
	}

	// https://github.com/nodejs/http-parser
	http_parser parser;

	// aditional parser state

	// headers
	std::string field;
	std::string value;
	bool valueValid = false;
	Headers headers;

	// server mode: url without protocol and host (e.g. "/foo/bar?foo=bar")
	// client mode: status (e.g. "OK")
	std::string urlOrStatus;
};
