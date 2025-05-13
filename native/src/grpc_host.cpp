#include <wasm_export.h>
#include <string.h>
#include <stdio.h>
#include <string>
#include <thread>

#include "config.h"
#include "wasm_context.h"
#include "call_wasm.h"
#include "spawn_threads_wasm.h"

#include "message.pb.h"

void send_mymessage(wasm_exec_env_t exec_env, uint32_t offset, uint32_t length);
int32_t receive_mymessage(wasm_exec_env_t exec_env, uint32_t offset, uint32_t max_length);

int main() {
    WasmContext ctx = initialize_wasm();
    if (!ctx.ok) {
      fprintf(stderr, "Failed to init WASM: %s\n", ctx.error.c_str());
      return 1;
    }

    printf("not launching threads\n");

    printf(">> client_module_inst = %p\n", (void*)ctx.client_module_inst);
    if (!ctx.client_module_inst) {
      fputs("ERROR: client_module_inst is NULL!\n", stderr);
      return 1;
    }

    auto func = wasm_runtime_lookup_function(
      ctx.client_module_inst, "_start");
    if (!func) {
        std::fprintf(stderr, "lookup failed\n");
        return 1;
    }
    call_no_args(ctx.client_exec_env, ctx.client_module_inst, func);

    auto func2 = wasm_runtime_lookup_function(
      ctx.server_module_inst, "_start");
    if (!func2) {
        std::fprintf(stderr, "lookup failed\n");
        return 1;
    }
    call_no_args(ctx.server_exec_env, ctx.server_module_inst, func2);
    
    // wasm_exec_env_t server_thread_env = wasm_runtime_spawn_exec_env(ctx.server_exec_env);
    // wasm_exec_env_t client_thread_env = wasm_runtime_spawn_exec_env(ctx.client_exec_env);
    // printf("made thread envs\n");
    // if (!server_thread_env || !client_thread_env) {
    //   std::fprintf(stderr, "spawn_exec_env failed\n");
    //   return 1;
    // }

    // ThreadArgs server_args = ThreadArgs::start(server_thread_env);
    // ThreadArgs client_args = ThreadArgs::start(client_thread_env);

    // printf("running threads\n");
    // std::thread server_thread{run_wasm_function, std::cref(server_args)};    
    // std::thread client_thread{run_wasm_function, std::cref(client_args)};    
    
    // // wait for threads to finish
    // server_thread.join();
    // client_thread.join();
    
    return 0;
}