// wasm_context.h
#pragma once

#include <wasm_export.h>
#include <vector>

struct ContextDimensions {
    int stack_size;
    int heap_size;
    int error_buf_size;
    int max_threads;
};

struct WasmContext {
    wasm_module_t        client_module;
    wasm_module_inst_t   client_module_inst;
    wasm_exec_env_t      client_exec_env;
    wasm_module_t        server_module;
    wasm_module_inst_t   server_module_inst;
    wasm_exec_env_t      server_exec_env;
    ContextDimensions    dimensions;
    bool                 ok;
    std::string          error;
    std::vector<uint8_t> client_bytes;
    std::vector<uint8_t> server_bytes;
};

WasmContext initialize_wasm();