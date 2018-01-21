include_directories(".")
add_definitions(-DASIO_STANDALONE)

#add_definitions(-DDEBUG_PROTOCOL)
add_definitions(-DDEBUG_NETWORK)

add_executable(${PROJECT_NAME}
	main.cpp
	asio.h
	cast.h
	Channel.cpp
	Channel.h
	Gateway.cpp
	Gateway.h
	HttpChannel.cpp
	HttpChannel.h
	Object.cpp
	Object.h
	opt.h
	Parameters.h
	Parameters.cpp
	ptr.h
	Server.cpp
	Server.h
	ZWaveNetwork.cpp
	ZWaveNetwork.h
	ZWaveProtocol.cpp
	ZWaveProtocol.h
	nodes/FibaroFgr222.cpp
	nodes/FibaroFgr222.h
	http-parser/http_parser.c
)
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})
