#!/usr/bin/env bash
exec "${PWD}/.venv/bin/python" \
     "${PWD}/third_party/nanopb/generator/nanopb_generator.py" \
     --protoc-plugin "$@"
