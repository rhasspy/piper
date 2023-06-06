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

RUN mkdir -p "lib/Linux-$(uname -m)"

ARG ONNXRUNTIME_VERSION='1.14.1'
RUN if [ "${TARGETARCH}${TARGETVARIANT}" = 'amd64' ]; then \
        ONNXRUNTIME_ARCH='x64'; \
    else \
        ONNXRUNTIME_ARCH="$(uname -m)"; \
    fi && \
    curl -L "https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/onnxruntime-linux-${ONNXRUNTIME_ARCH}-${ONNXRUNTIME_VERSION}.tgz" | \
        tar -C "lib/Linux-$(uname -m)" -xzvf - && \
    mv "lib/Linux-$(uname -m)"/onnxruntime-* \
       "lib/Linux-$(uname -m)/onnxruntime"

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
    cp -dR /build/build/*.so* /build/build/espeak-ng-data /build/build/piper ./piper/ && \
    tar -czf "piper_${TARGETARCH}${TARGETVARIANT}.tar.gz" piper/

# -----------------------------------------------------------------------------

FROM scratch

COPY --from=build /dist/piper_*.tar.gz ./
