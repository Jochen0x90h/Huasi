.PHONY : release debug openwrt clean

release:
	(mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j)

debug:
	(mkdir -p build-debug && cd build-debug && cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j)

openwrt:
	(mkdir -p build-openwrt && cd build-openwrt && cmake -DCMAKE_TOOLCHAIN_FILE=../openwrt.cmake -DCMAKE_BUILD_TYPE=Release .. && make -j)

clean:
	(rm -rf build)
