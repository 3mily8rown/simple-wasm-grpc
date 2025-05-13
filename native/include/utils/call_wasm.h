// host_utils.h
#ifndef HOST_UTILS_H
#define HOST_UTILS_H

#include "wasm_export.h"

bool call_no_args(wasm_exec_env_t exec_env,
    wasm_module_inst_t inst,
    wasm_function_inst_t func);

#endif 
