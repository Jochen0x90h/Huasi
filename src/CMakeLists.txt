include_directories(".")
add_definitions(-DASIO_STANDALONE)

#add_definitions(-DDEBUG_PROTOCOL)
add_definitions(-DDEBUG_NETWORK)

set(SOURCES
	main.cpp
	asio.hpp
	cast.hpp
	Channel.cpp
	Channel.hpp
	Gateway.cpp
	Gateway.hpp
	Object.cpp
	Object.hpp
	optional.hpp
	Parameters.hpp
	Parameters.cpp
	ptr.hpp
	Server.cpp
	Server.hpp
	Network.cpp
	Network.hpp
)
source_group(Sources FILES ${SOURCES})

set(ZWAVE
	zwave/ZWaveNetwork.cpp
	zwave/ZWaveNetwork.hpp
	zwave/ZWaveProtocol.cpp
	zwave/ZWaveProtocol.hpp
	zwave/FibaroFgr222.cpp
	zwave/FibaroFgr222.hpp
)
source_group(ZWave FILES ${ZWAVE})

set(ENOCEAN
	enocean/EnOceanProtocol.cpp
	enocean/EnOceanProtocol.hpp
	enocean/EnOceanNetwork.cpp
	enocean/EnOceanNetwork.hpp
)
source_group(EnOcean FILES ${ENOCEAN})

set(HTTP
	http/http_parser.c
	http/http_parser.h
	http/HttpChannel.cpp
	http/HttpChannel.hpp
	http/Url.hpp
)
source_group(HTTP FILES ${HTTP})

add_executable(${PROJECT_NAME}
	${SOURCES}
	${ZWAVE}
	${ENOCEAN}
	${HTTP}
)
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})
