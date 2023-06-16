FROM quay.io/pypa/manylinux_2_28_x86_64 as build-amd64

FROM quay.io/pypa/manylinux_2_28_aarch64 as build-arm64

FROM debian:bullseye as build-armv7

RUN apt-get update && \
    apt-get install --yes --no-install-recommends \
        build-essential cmake ca-certificates curl pkg-config

# -----------------------------------------------------------------------------

ARG TARGETARCH
ARG TARGETVARIANT
FROM build-${TARGETARCH}${TARGETVARIANT} as build
ARG TARGETARCH
ARG TARGETVARIANT

ENV LANG C.UTF-8
ENV DEBIAN_FRONTEND=noninteractive

WORKDIR /build

ARG SPDLOG_VERSION="1.11.0"
RUN curl -L "https://github.com/gabime/spdlog/archive/refs/tags/v${SPDLOG_VERSION}.tar.gz" | \
    tar -xzvf - && \
    mkdir -p "spdlog-${SPDLOG_VERSION}/build" && \
    cd "spdlog-${SPDLOG_VERSION}/build" && \
    cmake ..  && \
    make -j8 && \
    cmake --install . --prefix /usr

RUN mkdir -p "lib/Linux-$(uname -m)"

# Use pre-compiled Piper phonemization library (includes onnxruntime)
ARG PIPER_PHONEMIZE_VERSION='1.0.0'
RUN mkdir -p "lib/Linux-$(uname -m)/piper_phonemize" && \
    curl -L "https://github.com/rhasspy/piper-phonemize/releases/download/v${PIPER_PHONEMIZE_VERSION}/libpiper_phonemize-${TARGETARCH}${TARGETVARIANT}.tar.gz" | \
        tar -C "lib/Linux-$(uname -m)/piper_phonemize" -xzvf -

# Build piper binary
COPY Makefile ./
COPY src/cpp/ ./src/cpp/
RUN make

# Do a test run
RUN ./build/piper --help

# Build .tar.gz to keep symlinks
WORKDIR /dist
RUN mkdir -p piper && \
    cp -dR /build/build/*.so* /build/build/espeak-ng-data /build/build/libtashkeel_model.ort /build/build/piper ./piper/ && \
    tar -czf "piper_${TARGETARCH}${TARGETVARIANT}.tar.gz" piper/

# -----------------------------------------------------------------------------

FROM debian:bullseye as test
ARG TARGETARCH
ARG TARGETVARIANT

WORKDIR /test

COPY local/en-us/lessac/low/en-us-lessac-low.onnx \
     local/en-us/lessac/low/en-us-lessac-low.onnx.json ./

# Run Piper on a test sentence and verify that the WAV file isn't empty
COPY --from=build /dist/piper_*.tar.gz ./
RUN tar -xzf piper*.tar.gz
RUN echo 'This is a test.' | ./piper/piper -m en-us-lessac-low.onnx -f test.wav
RUN if [ ! -f test.wav ]; then exit 1; fi
RUN size="$(wc -c < test.wav)"; \
    if [ "${size}" -lt "1000" ]; then echo "File size is ${size} bytes"; exit 1; fi

# -----------------------------------------------------------------------------

FROM scratch

COPY --from=test /test/piper_*.tar.gz /test/test.wav ./
