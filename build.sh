#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENV_FILE="${DIR}/config/paths.env"
source "$ENV_FILE"

BUILD_STEP=${1:-all}      # wasm, host, all, skip
HOST_MODE=${2:-full}      # full, build, debug
COMPILE_PROTO=${3:-proto} # proto, skip  

if [[ "$HOST_MODE" != "build" && "$BUILD_STEP" != "wasm" ]]; then
    cd "${NATIVE_DIR}"
    rm -rf "${BUILD_DIR}" cmake-build-debug CMakeCache.txt CMakeFiles
    mkdir -p "${WASM_OUT}"
fi

"${SCRIPT_DIR}/native_scripts/third_party.sh"
"${SCRIPT_DIR}/wasm_scripts/third_party.sh"

if [ "$COMPILE_PROTO" = "proto" ]; then
    "${SCRIPT_DIR}/proto_scripts/setup_proto.sh"
fi

case "${BUILD_STEP}" in
  wasm)
    "${SCRIPT_DIR}/wasm_scripts/build_wasm.sh"
    ;;
  host)
    "${SCRIPT_DIR}/native_scripts/build_host.sh" "${HOST_MODE}"
    ;;
  all)
    "${SCRIPT_DIR}/wasm_scripts/build_wasm.sh"
    "${SCRIPT_DIR}/native_scripts/build_host.sh" "${HOST_MODE}"
    ;;
esac