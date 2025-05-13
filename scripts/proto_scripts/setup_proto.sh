#!/bin/bash
set -e
# requires protobuf, abseil and nanopb to be installed and at the paths in config/paths.env

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENV_FILE="${DIR}/../../config/paths.env"
source "$ENV_FILE"

cd "${ROOT_DIR}"

echo "##################### Running setup_proto.sh"

cd "$ROOT_DIR"

# Generate proto files with the built protoc

PROTO_FILE="${PROTO_DIR}/message.proto"

OUT_CPP="${PROTO_DIR}/generated_full"

mkdir -p "${OUT_CPP}"

echo "Generating full Protobuf C++ files..."
"${PROTOC_BIN}" \
    --proto_path="${PROTO_DIR}" \
    --cpp_out="${OUT_CPP}" \
    "${PROTO_FILE}"


# Generate the proto files for the wasm modules with the built protoc

# Set up venv if needed
VENV_DIR="$ROOT_DIR/.venv"
if [ ! -d "$VENV_DIR" ]; then
  echo "Creating Python virtual environment..."
  python3 -m venv "$VENV_DIR"
  "$VENV_DIR/bin/pip" install --upgrade pip
  "$VENV_DIR/bin/pip" install protobuf grpcio-tools
fi

export PATH="$VENV_DIR/bin:$PATH"

# Inside proto dir to help nanopb find the options file (it wouldn't work with a flag)
cd $PROTO_DIR 

OUT_NANO=generated_nano
PROTO_NANO_FILE=message.proto

mkdir -p "$OUT_NANO"

echo "Generating Nanopb C files..."

rm -rf "$OUT_NANO"/*

$PROTOC_BIN \
  --plugin=protoc-gen-nanopb="${WASM_NANOPB}/nanopb_plugin.sh" \
  --proto_path=. \
  --proto_path="${NANOPB}/generator/proto" \
  --nanopb_out="${OUT_NANO}" \
  "$PROTO_NANO_FILE"
