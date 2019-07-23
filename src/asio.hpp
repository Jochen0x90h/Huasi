#pragma once

#include "asio/io_service.hpp"
#include "asio/serial_port.hpp"
#include "asio/ip/tcp.hpp"
#include "asio/steady_timer.hpp"
#include "asio/write.hpp"

using std::error_code;
using std::error_category;

inline bool isCanceled(std::error_code error) {
	return error.category() == asio::error::get_system_category() && error.value() == asio::error::operation_aborted;
}

inline bool isEof(std::error_code error) {
	return error.category() == asio::error::get_misc_category() && error.value() == asio::error::eof;
}
