#!/bin/bash

set -e  # Exit immediately on any error

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENV_FILE="${DIR}/../../config/paths.env"
source "$ENV_FILE"

STACK_SIZE=${WASM_STACK_SIZE:-262144}

echo "##################### build wasm apps"

mkdir -p "${WASM_OUT}" "${GEN_DIR}"
cd "${WASM_APPS}"

for src in *.cpp
do
  BASE="${src%.*}"
  OUT_FILE="${WASM_OUT}/${BASE}.wasm"

  echo "building ${OUT_FILE} from ${src}"
  # could use -Wl,--allow-undefined \ instead of a file listing functions
  # -Wl,--allow-undefined-file="${WASM_DIR}/native_impls.txt" \
  $CXX \
          --target=wasm32-wasi \
          -I"${PROTO_DIR}/generated_nano" \
          -I"${NANOPB}" \
          -L"${WASM_NANOPB}/build" \
          -lnanopb \
          -Wl,--export-all \
          -Wl,--no-gc-sections \
          -Wl,--allow-undefined \
          -o "${OUT_FILE}" \
          "${src}" \
          "${PROTO_DIR}/generated_nano/message.pb.c"

  # # generate export list header
  # echo "Generating exports header ${BASE}"
  # python3 "${SCRIPT_DIR}/wasm_scripts/generate_wasm_exports.py" \
  #   "${OUT_FILE}" "${GEN_DIR}/${BASE}_exports.h"

  # generate import
  echo "Generating imports header for ${BASE}"
  rm -f "${GEN_DIR}/${BASE}_imports.h"
  
  python3 "${SCRIPT_DIR}/wasm_scripts/generate_wasm_imports.py" \
      --module "${BASE}" \
      "${OUT_FILE}" "${GEN_DIR}/${BASE}_imports.h"

done

echo "####################build wasm apps done"