// include/spawn_threads_wasm.h
#pragma once

#include <vector>
#include <string>
#include <wasm_export.h>

struct ThreadArgs {
    wasm_exec_env_t exec_env;
    std::string     func_name;

    ThreadArgs(wasm_exec_env_t e, std::string name)
      : exec_env(e), func_name(std::move(name))
    {}

    static ThreadArgs start(wasm_exec_env_t e) {
        return ThreadArgs(e, "_start");
    }
};

void run_wasm_function(const ThreadArgs& args);