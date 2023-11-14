.PHONY: clean docker

all:
	cmake -Bbuild -DCMAKE_INSTALL_PREFIX=install
	cmake --build build --config Release
	cd build && ctest --config Release
	cmake --install build

docker:
	docker buildx build . --platform linux/amd64,linux/arm64,linux/arm/v7 --output 'type=local,dest=dist'

clean:
	rm -rf build install dist
