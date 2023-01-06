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
        cmake \
        pkg-config \
        libespeak-ng-dev \
        libpcaudio-dev

WORKDIR /build

# Copy onnxruntime library
COPY lib/ ./lib/
RUN mkdir -p /usr/local/include/onnxruntime && \
    tar -C /usr/local/include/onnxruntime \
        --strip-components 1 \
        -xvf "lib/onnxruntime-${TARGETARCH}${TARGETVARIANT}.tgz"

# Build larynx binary
COPY Makefile ./
COPY src/cpp/ ./src/cpp/
RUN make release

# -----------------------------------------------------------------------------

FROM scratch

COPY --from=build /usr/local/include/onnxruntime/lib/libonnxruntime.so.* ./
COPY --from=build /build/build/larynx ./
