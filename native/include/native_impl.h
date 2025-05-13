// native_impls.h
#pragma once

#include <cstdint>
#include <wasm_export.h>
#include <string>

void send_mymessage(wasm_exec_env_t exec_env, uint32_t offset,
                    uint32_t length);

int32_t receive_mymessage(wasm_exec_env_t exec_env, uint32_t offset,
    uint32_t max_length);
    
