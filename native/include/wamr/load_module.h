// include/load_module.h
#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <wasm_export.h>   // for wasm_module_t, wasm_module_inst_t, wasm_exec_env_t

// Read an entire file into a byte buffer.
// Throws std::runtime_error on failure.
std::vector<uint8_t> readFileToBytes(const std::string& path);

// Load a Wasm module from its bytecode buffer, instantiate it, and create an exec env.
//
// buffer           : the raw Wasm bytes
// out_inst         : set to the created module instance
// out_env          : set to the created exec environment
// stack_size       : size of the Wasm stack for this module
// heap_size        : size of the Wasm heap for this module
// error_buf        : buffer to receive any error message
// error_buf_size   : size of that buffer
//
// Returns the loaded wasm_module_t on success, or nullptr on failure.
wasm_module_t load_module_minimal(
    std::vector<uint8_t>&       buffer,
    wasm_module_inst_t&         out_inst,
    wasm_exec_env_t&            out_env,
    uint32_t                    stack_size,
    uint32_t                    heap_size,
    char*                       error_buf,
    size_t                      error_buf_size);
