// wasm_threads.cpp

#include <wasm_export.h>
#include <cstdio>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "spawn_threads_wasm.h"
#include "call_wasm.h"

void run_wasm_start(ThreadArgs& args) {
    args.func_name = "_start";
    run_wasm_function(args);
}

// thread worker: for no return and no parameters
void run_wasm_function(const ThreadArgs& args) {
    printf("[run_wasm_function] start, exec_env=%p, funcName=%s\n",
        (void*)args.exec_env, args.func_name.c_str());

    if (!wasm_runtime_init_thread_env()) {
        std::fprintf(stderr, "[%s] failed to init thread env\n",
                     args.func_name.c_str());
        return;
    }

    // lookup module instance and function
    auto module_inst = wasm_runtime_get_module_inst(args.exec_env);
    auto func = wasm_runtime_lookup_function(
        module_inst, args.func_name.c_str());
    if (!func) {
        std::fprintf(stderr, "[%s] lookup failed\n", args.func_name.c_str());
        wasm_runtime_destroy_thread_env();
        return;
    }

    // call
    if (!call_no_args(args.exec_env, module_inst, func)) {
        // TODO handle the trap…
    }

    // tear down
    wasm_runtime_destroy_thread_env();
    wasm_runtime_destroy_spawned_exec_env(args.exec_env);
}
