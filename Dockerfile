# Stage 1: Builder
FROM docker.io/silkeh/clang:21-trixie AS builder

ADD --chmod=644 https://apt.llvm.org/llvm-snapshot.gpg.key /etc/apt/trusted.gpg.d/apt.llvm.org.asc

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        git \
        curl \
        zip \
        unzip \
        tar \
        ca-certificates \
        cmake \
        ninja-build \
        bison \ 
        flex \
  pkg-config && \
    rm -rf /var/lib/apt/lists/*
    
ARG CMAKE_VERSION=3.31.10
RUN curl -sSL https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.sh -o /tmp/cmake.sh && \
    chmod +x /tmp/cmake.sh && \
    /tmp/cmake.sh --prefix=/usr/local --skip-license && \
    rm /tmp/cmake.sh
    
#VCPKG setup
ENV VCPKG_ROOT=/app/vcpkg
ENV VCPKG_INSTALLED_DIR=/app/vcpkg_installed
ENV PATH="${PATH}:${VCPKG_ROOT}"

ENV VCPKG_TRIPLET=x64-linux
ENV VCPKG_USE_SYSTEM_BINARIES=1

RUN git clone --depth=1 https://github.com/microsoft/vcpkg.git $VCPKG_ROOT && \
    $VCPKG_ROOT/bootstrap-vcpkg.sh -disableMetrics
    
WORKDIR /app

COPY ./vcpkg.json /app/vcpkg.json
RUN mkdir -p /var/cache/vcpkg
RUN --mount=type=cache,target=/var/cache/vcpkg \
    VCPKG_DEFAULT_BINARY_CACHE=/var/cache/vcpkg $VCPKG_ROOT/vcpkg install --triplet x64-linux --clean-after-build

COPY CMakeLists.txt /app/CMakeLists.txt
COPY src/ /app/src/
COPY cmake/ /app/cmake/
COPY libs /app/libs/

RUN git clone --depth=1 https://github.com/sprotas-daml/daml-sdk-cpp.git /app/libs/daml-sdk

# Configure and build the application

RUN cmake -S . -B build -G Ninja \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_TARGET_TRIPLET=x64-linux\
    -DVCPKG_INSTALLED_DIR=$VCPKG_INSTALLED_DIR

RUN cmake --build build && strip build/bin/exe


# Stage 2: Runner
FROM docker.io/library/debian:trixie-slim

ENV DEBIAN_FRONTEND=noninteractive
ENV LOCAL_NODE_CONFIG=/app/config.local.toml

WORKDIR /app

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    ca-certificates \
    libatomic1 && \
    rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/build/bin/exe /app/exe

COPY config.local.toml /app/config.local.toml

CMD ["/app/exe"]
