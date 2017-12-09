.PHONY : release debug clean

release:
	(mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j)

debug:
	(mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j)

clean:
	(rm -rf build)
