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
ADD lib/espeak-ng-1.51.tar.gz ./
RUN cd espeak-ng-1.51 && \
    ./autogen.sh && \
    ./configure \
        --without-pcaudiolib \
        --without-klatt \
        --without-speechplayer \
        --without-mbrola \
        --without-sonic \
        --prefix=/usr && \
    make -j8 src/espeak-ng src/speak-ng && \
    make && \
    make install

# Copy onnxruntime library
COPY lib/ ./lib/
RUN mkdir -p /usr/local/include/onnxruntime && \
    tar -C /usr/local/include/onnxruntime \
        --strip-components 1 \
        -xvf "lib/onnxruntime-linux-${TARGETARCH}${TARGETVARIANT}.tgz"

# Build larynx binary
COPY Makefile ./
COPY src/cpp/ ./src/cpp/
RUN make no-pcaudio

# Do a test run
RUN /build/build/larynx --help

# Build .tar.gz to keep symlinks
WORKDIR /dist
RUN mkdir -p larynx && \
    cp -d /usr/lib64/libespeak-ng.so* ./larynx/ && \
    cp -dR /usr/share/espeak-ng-data ./larynx/ && \
    cp -d /usr/local/include/onnxruntime/lib/libonnxruntime.so.* ./larynx/ && \
    cp /build/build/larynx ./larynx/ && \
    tar -czf "larynx_${TARGETARCH}${TARGETVARIANT}.tar.gz" larynx/

# -----------------------------------------------------------------------------

FROM scratch

COPY --from=build /dist/larynx_*.tar.gz ./
