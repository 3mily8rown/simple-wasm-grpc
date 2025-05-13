#!/bin/bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENV_FILE="${DIR}/../../config/paths.env"
source "$ENV_FILE"

mkdir -p ${WASM_THIRD_PARTY}
cd ${WASM_THIRD_PARTY}

if [ ! -d nanopb ]; then
  git clone --depth 1 https://github.com/nanopb/nanopb.git
  cd "${WASM_NANOPB}"
  #cross compile to wasm   
  "./nanopb_build.sh"
fi