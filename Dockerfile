FROM quay.io/pypa/manylinux_2_28_x86_64 as build-amd64

FROM quay.io/pypa/manylinux_2_28_aarch64 as build-arm64

ARG TARGETARCH
ARG TARGETVARIANT
FROM build-${TARGETARCH}${TARGETVARIANT} as build
ARG TARGETARCH
ARG TARGETVARIANT

ENV LANG C.UTF-8
ENV DEBIAN_FRONTEND=noninteractive

WORKDIR /build

# Build minimal version of espeak-ng
ADD lib/espeak-ng-1.52-patched.tar.gz ./
RUN cd espeak-ng && \
    ./autogen.sh && \
    ./configure \
        --without-pcaudiolib \
        --without-klatt \
        --without-speechplayer \
        --without-mbrola \
        --without-sonic \
        --with-extdict-cmn \
        --prefix=/usr && \
    make -j8 src/espeak-ng src/speak-ng && \
    make && \
    make install

# Copy onnxruntime library
COPY lib/onnxruntime-linux-*.tgz ./lib/
RUN export ONNX_DIR="./lib/Linux-$(uname -m)" && \
    mkdir -p "${ONNX_DIR}" && \
    tar -C "${ONNX_DIR}" \
        --strip-components 1 \
        -xvf "lib/onnxruntime-linux-${TARGETARCH}${TARGETVARIANT}.tgz"

# Build piper binary
COPY Makefile ./
COPY src/cpp/ ./src/cpp/
RUN make no-pcaudio

# Do a test run
RUN /build/build/piper --help

# Build .tar.gz to keep symlinks
WORKDIR /dist
RUN mkdir -p piper && \
    cp -d /usr/lib64/libespeak-ng.so* ./piper/ && \
    cp -dR /usr/share/espeak-ng-data ./piper/ && \
    find /build/lib/ -name 'libonnxruntime.so.*' -exec cp -d {} ./piper/ \; && \
    cp /build/build/piper ./piper/ && \
    tar -czf "piper_${TARGETARCH}${TARGETVARIANT}.tar.gz" piper/

# -----------------------------------------------------------------------------

FROM scratch

COPY --from=build /dist/piper_*.tar.gz ./
