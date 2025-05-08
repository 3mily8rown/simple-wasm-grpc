#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
GENERATOR="${SCRIPT_DIR}/../../third_party/nanopb/generator/nanopb_generator.py"

echo "Running nanopb plugin..." >&2
echo "Invoking: python3 \"$GENERATOR\" --protoc-plugin $@" >&2

# Force plugin mode by passing a flag
exec python3 "$GENERATOR" --protoc-plugin "$@"
