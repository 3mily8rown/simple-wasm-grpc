// host_utils.h
#ifndef HOST_UTILS_H
#define HOST_UTILS_H

#include "wasm_export.h"

int call_generate_and_format_float(wasm_exec_env_t exec_env, 
    wasm_module_inst_t module_inst);

int call_int_func(wasm_module_inst_t module_inst,
    wasm_exec_env_t exec_env,
    const char* func_name,
    const std::vector<uint32_t>& args,
    int32_t& result_out);

const char* call_func(wasm_module_inst_t module_inst, 
    wasm_exec_env_t env, 
    const char* func_name, 
    const char* arg = nullptr);

int call_cached_int_func(wasm_module_inst_t module_inst,
    wasm_exec_env_t exec_env,
    const std::string &name,
    const std::vector<uint32_t> &args,
    int32_t &result_out);

#endif 
