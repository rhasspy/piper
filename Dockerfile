FROM debian:bullseye as build
ARG TARGETARCH
ARG TARGETVARIANT

ENV LANG C.UTF-8
ENV DEBIAN_FRONTEND=noninteractive

RUN echo "Dir::Cache var/cache/apt/${TARGETARCH}${TARGETVARIANT};" > /etc/apt/apt.conf.d/01cache

RUN --mount=type=cache,id=apt-build,target=/var/cache/apt \
    mkdir -p /var/cache/apt/${TARGETARCH}${TARGETVARIANT}/archives/partial && \
    apt-get update && \
    apt-get install --yes --no-install-recommends \
        build-essential \
        autoconf automake libtool pkg-config cmake

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
        -xvf "lib/onnxruntime-${TARGETARCH}${TARGETVARIANT}.tgz"

# Build larynx binary
COPY Makefile ./
COPY src/cpp/ ./src/cpp/
RUN make no-pcaudio

# Build .tar.gz to keep symlinks
WORKDIR /dist
RUN mkdir -p larynx && \
    cp -d /usr/lib/libespeak-ng.so* ./larynx/ && \
    cp -dR /usr/share/espeak-ng-data ./larynx/ && \
    cp -d /usr/local/include/onnxruntime/lib/libonnxruntime.so.* ./larynx/ && \
    cp /build/build/larynx ./larynx/ && \
    tar -czf larynx.tar.gz larynx/

# -----------------------------------------------------------------------------

FROM scratch

COPY --from=build /dist/larynx.tar.gz ./
