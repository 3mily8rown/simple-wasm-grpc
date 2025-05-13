// native_impls.h
#pragma once

#include <cstdint>
#include <wasm_export.h>   // for wasm_exec_env_t, wasm_module_inst_t, etc.
#include <string>

void pass_to_native(wasm_exec_env_t exec_env, uint32_t offset,
                    uint32_t length);

void pass_to_native_rpc(wasm_exec_env_t exec_env, uint32_t offset,
    uint32_t length);
    
