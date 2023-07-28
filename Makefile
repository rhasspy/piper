.PHONY: piper clean

LIB_DIR := lib/Linux-$(shell uname -m)
VERSION := $(cat VERSION)
DOCKER_PLATFORM ?= linux/amd64,linux/arm64,linux/arm/v7

piper:
	mkdir -p build
	cd build && cmake ../src/cpp -DCMAKE_BUILD_TYPE=Release && make
	cp -aR $(LIB_DIR)/piper_phonemize/lib/espeak-ng-data $(LIB_DIR)/piper_phonemize/lib/*.so* $(LIB_DIR)/piper_phonemize/etc/* build/

clean:
	rm -rf build/ dist/

docker:
	docker buildx build . --platform '$(DOCKER_PLATFORM)' --output 'type=local,dest=dist'
