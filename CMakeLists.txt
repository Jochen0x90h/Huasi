cmake_minimum_required(VERSION 3.9)

# set project (huasi = house in Quechua)
project(huasi)

# use C++11 standard by default
set(CMAKE_CXX_STANDARD 11)

find_package(Threads)

set(LIBRARIES
	${CMAKE_THREAD_LIBS_INIT}
)

add_subdirectory(src)
