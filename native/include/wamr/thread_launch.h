#pragma once

#include <pthread.h>
#include <wasm_export.h>

#include "wasm/call_wasm.h"  

bool start_wasm_thread(WASMModuleInstanceCommon *module_inst,
    wasm_function_inst_t func,
    pthread_t *out_thread);
