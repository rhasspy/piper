.PHONY: release debug clean

release:
	mkdir -p build
	cd build && cmake ../src/cpp -DCMAKE_BUILD_TYPE=Release && make

debug:
	mkdir -p build
	cd build && cmake ../src/cpp -DCMAKE_BUILD_TYPE=Debug && make

clean:
	rm -rf build/ dist/
