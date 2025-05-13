// host_utils.h
#ifndef HOST_UTILS_H
#define HOST_UTILS_H

#include <vector>
#include <string>

#include "wasm_export.h"

bool call_no_args(wasm_exec_env_t exec_env,
    wasm_module_inst_t inst,
    wasm_function_inst_t func);

int call_cached_int_func(wasm_module_inst_t module_inst,
    wasm_exec_env_t exec_env,
    const std::string &name,
    const std::vector<uint32_t> &args,
    int32_t &result_out);

#endif 
