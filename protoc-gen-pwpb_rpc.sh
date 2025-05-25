#!/usr/bin/env bash
export PYTHONPATH="${PWD}/third_party/pigweed:${PWD}/.venv/lib/python3.10/site-packages:$PYTHONPATH"
exec "${PWD}/.venv/bin/python" -m pw_protobuf_compiler.protoc_gen_pwpb_rpc "$@"
