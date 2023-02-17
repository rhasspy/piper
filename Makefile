.PHONY: release debug clean test

release:
	mkdir -p build
	cd build && cmake ../src/cpp -DCMAKE_BUILD_TYPE=Release && make

no-pcaudio:
	mkdir -p build
	cd build && cmake ../src/cpp -DCMAKE_BUILD_TYPE=Release -DUSE_PCAUDIO=OFF && make

debug:
	mkdir -p build
	cd build && cmake ../src/cpp -DCMAKE_BUILD_TYPE=Debug && make

clean:
	rm -rf build/ dist/

docker:
	docker buildx build . --platform 'linux/amd64,linux/arm64' --output 'type=local,dest=dist'

test:
	docker buildx build -f Dockerfile.test . --platform 'linux/amd64,linux/arm64'
