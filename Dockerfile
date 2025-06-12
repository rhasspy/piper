FROM debian:bullseye AS build
ARG TARGETARCH
ARG TARGETVARIANT

ENV LANG=C.UTF-8
ENV DEBIAN_FRONTEND=noninteractive

# パッケージリストの更新とインストールを分離
RUN set -e; \
    echo 'Acquire::Retries "3";' > /etc/apt/apt.conf.d/80-retries; \
    # -- 基本ツール --
    for i in 1 2 3; do \
      apt-get update && \
      apt-get install --yes --no-install-recommends \
        ca-certificates curl gnupg lsb-release && break || { echo "apt preinstall failed ($i)"; sleep 5; }; \
    done; \
    # docker repo key
    curl -fsSL https://download.docker.com/linux/debian/gpg | gpg --dearmor -o /usr/share/keyrings/docker-archive-keyring.gpg; \
    echo "deb [signed-by=/usr/share/keyrings/docker-archive-keyring.gpg] https://download.docker.com/linux/debian bullseye stable" | tee /etc/apt/sources.list.d/docker.list > /dev/null; \
    for i in 1 2 3; do \
      apt-get update && \
      apt-get install --yes --no-install-recommends \
        build-essential cmake git pkg-config libicu-dev libespeak-ng-dev make ninja-build python3 ccache && break || { echo "apt core install failed ($i)"; sleep 5; }; \
    done; \
    # --- クロスコンパイルツール ----
    # buildx/QEMU ではビルド用コンテナ自体がターゲットと同じアーキテクチャになる。
    # その場合追加ツールは不要で、Debian arm リポジトリには crossbuild-essential-* が存在しない。
    #   ・ホスト = amd64 かつ TARGETARCH が異なるときだけインストールする。
    HOST_ARCH=$(dpkg --print-architecture); \
    if [ "$HOST_ARCH" = "amd64" ] && [ "$TARGETARCH" = "arm64" ]; then \
        apt-get install --yes --no-install-recommends \
            gcc-aarch64-linux-gnu g++-aarch64-linux-gnu binutils-aarch64-linux-gnu; \
        ln -s /usr/bin/ccache /usr/local/bin/aarch64-linux-gnu-gcc; \
        ln -s /usr/bin/ccache /usr/local/bin/aarch64-linux-gnu-g++; \
    elif [ "$HOST_ARCH" = "amd64" ] && [ "$TARGETARCH" = "arm" ] && [ "$TARGETVARIANT" = "v7" ]; then \
        apt-get install --yes --no-install-recommends \
            gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf binutils-arm-linux-gnueabihf; \
        ln -s /usr/bin/ccache /usr/local/bin/arm-linux-gnueabihf-gcc; \
        ln -s /usr/bin/ccache /usr/local/bin/arm-linux-gnueabihf-g++; \
    fi && \
    # クリーンアップ
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /build

# ツールチェインファイルを作成
RUN mkdir -p cmake && \
    if [ "$TARGETARCH" = "arm64" ]; then \
        echo 'set(CMAKE_SYSTEM_NAME Linux)' > cmake/linux-aarch64.cmake && \
        echo 'set(CMAKE_SYSTEM_PROCESSOR aarch64)' >> cmake/linux-aarch64.cmake && \
        echo 'set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)' >> cmake/linux-aarch64.cmake && \
        echo 'set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)' >> cmake/linux-aarch64.cmake && \
        echo 'set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu)' >> cmake/linux-aarch64.cmake && \
        echo 'set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)' >> cmake/linux-aarch64.cmake && \
        echo 'set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)' >> cmake/linux-aarch64.cmake && \
        echo 'set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)' >> cmake/linux-aarch64.cmake && \
        echo 'set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)' >> cmake/linux-aarch64.cmake; \
    elif [ "$TARGETARCH" = "arm" ] && [ "$TARGETVARIANT" = "v7" ]; then \
        echo 'set(CMAKE_SYSTEM_NAME Linux)' > cmake/linux-armv7.cmake && \
        echo 'set(CMAKE_SYSTEM_PROCESSOR armv7)' >> cmake/linux-armv7.cmake && \
        echo 'set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)' >> cmake/linux-armv7.cmake && \
        echo 'set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)' >> cmake/linux-armv7.cmake && \
        echo 'set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv7-a -mfpu=neon -mfloat-abi=hard")' >> cmake/linux-armv7.cmake && \
        echo 'set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv7-a -mfpu=neon -mfloat-abi=hard")' >> cmake/linux-armv7.cmake && \
        echo 'set(CMAKE_FIND_ROOT_PATH /usr/arm-linux-gnueabihf)' >> cmake/linux-armv7.cmake && \
        echo 'set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)' >> cmake/linux-armv7.cmake && \
        echo 'set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)' >> cmake/linux-armv7.cmake && \
        echo 'set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)' >> cmake/linux-armv7.cmake && \
        echo 'set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)' >> cmake/linux-armv7.cmake; \
    fi

# ソースコードをコピー
COPY ./ ./

# アーキテクチャに応じたビルド設定
RUN if [ "$TARGETARCH" = "amd64" ]; then \
        cmake -Bbuild -DCMAKE_INSTALL_PREFIX=install \
              -DCMAKE_C_COMPILER_LAUNCHER=ccache \
              -DCMAKE_CXX_COMPILER_LAUNCHER=ccache && \
        cmake --build build --config Release && \
        cmake --install build; \
    elif [ "$TARGETARCH" = "arm64" ]; then \
        cmake -Bbuild -DCMAKE_INSTALL_PREFIX=install \
            -DCMAKE_TOOLCHAIN_FILE=cmake/linux-aarch64.cmake \
            -DCMAKE_C_COMPILER_LAUNCHER=ccache \
            -DCMAKE_CXX_COMPILER_LAUNCHER=ccache && \
        cmake --build build --config Release && \
        cmake --install build; \
    elif [ "$TARGETARCH" = "arm" ] && [ "$TARGETVARIANT" = "v7" ]; then \
        cmake -Bbuild -DCMAKE_INSTALL_PREFIX=install \
            -DCMAKE_TOOLCHAIN_FILE=cmake/linux-armv7.cmake \
            -DCMAKE_C_COMPILER_LAUNCHER=ccache \
            -DCMAKE_CXX_COMPILER_LAUNCHER=ccache && \
        cmake --build build --config Release && \
        cmake --install build; \
    else \
        echo "Unsupported architecture: $TARGETARCH$TARGETVARIANT" && exit 1; \
    fi

# テスト実行（amd64のみ）
RUN if [ "$TARGETARCH" = "amd64" ]; then \
        ./build/piper --help; \
    fi

# アーカイブの作成
WORKDIR /dist
RUN mkdir -p piper && \
    cp -dR /build/install/* ./piper/ && \
    if [ "$TARGETARCH" = "arm" ] && [ "$TARGETVARIANT" = "v7" ]; then \
        tar -czf "piper_armv7.tar.gz" piper/; \
    else \
        tar -czf "piper_${TARGETARCH}.tar.gz" piper/; \
    fi

FROM scratch
COPY --from=build /dist/piper_*.tar.gz ./
