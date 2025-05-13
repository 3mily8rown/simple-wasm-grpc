#!/bin/bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENV_FILE="${DIR}/../../config/paths.env"
source "$ENV_FILE"

cd "${ROOT_DIR}"

# Clone abseil and protobuf if have not already for native side
mkdir -p ${THIRD_PARTY}
cd ${THIRD_PARTY}

if [ ! -d protobuf ]; then
  git clone -b v3.21.12 --depth 1 https://github.com/protocolbuffers/protobuf.git
fi
if [ ! -d abseil-cpp ]; then
  git clone -b 20230125.2 --depth 1 https://github.com/abseil/abseil-cpp.git
fi

cd "${PROTOBUF}"

if [ ! -d build ]; then
    mkdir -p build
    cd build
    cmake .. \
        -DCMAKE_C_COMPILER=/usr/bin/clang \
        -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
        -DCMAKE_BUILD_TYPE=Release \
        -Dprotobuf_BUILD_TESTS=OFF \
        -Dprotobuf_INSTALL=OFF \
        -Dprotobuf_BUILD_PROTOC_BINARIES=ON \
        -DCMAKE_C_FLAGS="-pthread" \
        -DCMAKE_CXX_FLAGS="-pthread"
    make -j$(nproc) protoc
fi

