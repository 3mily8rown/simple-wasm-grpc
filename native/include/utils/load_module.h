// include/load_module.h
#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <wasm_export.h>

// Read an entire file into a byte buffer.
std::vector<uint8_t> readFileToBytes(const std::string& path);

// Load a Wasm module from its bytecode buffer, instantiate it, and create an exec env.
wasm_module_t load_module_minimal(
    uint8_t* data,
    uint32_t size,
    wasm_module_inst_t& out_inst,
    wasm_exec_env_t& out_env,
    uint32_t stack_size,
    uint32_t heap_size,
    char* error_buf,
    size_t error_buf_size);
