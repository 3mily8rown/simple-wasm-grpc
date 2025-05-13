#include <cstdio>
#include <cstring>
#include <cstdint>
#include <string>
#include <pthread.h>
#include <wasm_export.h>

#include "load_module.h"
#include "config.h"
#include "call_wasm.h"
#include "thread_launch.h"

#include "client_app_imports.h"
#include "server_app_imports.h"

static const size_t all_env_native_symbols_count =
generated_client_app_native_symbols_count +
generated_server_app_native_symbols_count;

static NativeSymbol all_env_native_symbols[all_env_native_symbols_count];

void register_all_env_symbols() {
    size_t out_idx = 0;
    for (size_t i = 0; i < generated_client_app_native_symbols_count; ++i)
      all_env_native_symbols[out_idx++] = generated_client_app_native_symbols[i];
    for (size_t i = 0; i < generated_server_app_native_symbols_count; ++i)
      all_env_native_symbols[out_idx++] = generated_server_app_native_symbols[i];

    wasm_runtime_register_natives(
      "env",
      all_env_native_symbols,
      all_env_native_symbols_count
    );
}

int main() {
    // setting up wasm module
    wasm_runtime_set_log_level(WASM_LOG_LEVEL_DEBUG);
  
    // ------------------------------------------ per module
    wasm_module_t client_module = nullptr;
    wasm_module_inst_t client_module_inst = nullptr;
    wasm_exec_env_t client_exec_env = nullptr;

    wasm_module_t server_module = nullptr;
    wasm_module_inst_t server_module_inst = nullptr;
    wasm_exec_env_t server_exec_env = nullptr;

    // ------------------------------------------
    char error_buf[128];
    uint32_t buf_size, stack_size = 8092, heap_size = 8092;
  
    static char global_heap_buf[512 * 1024];
    RuntimeInitArgs init_args;
  
    memset(&init_args, 0, sizeof(RuntimeInitArgs));
    init_args.mem_alloc_type = Alloc_With_Pool;
    
    init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);
  
    if (!wasm_runtime_full_init(&init_args)) {
      printf("Init runtime environment failed.\n");
      return -1;
    }
  
    wasm_runtime_set_log_level(WASM_LOG_LEVEL_VERBOSE);
  
    register_all_env_symbols();

    // ------------------------------------------------------- load each module
  
    std::string client_wasm_path = Config::get("WASM_OUT") + "/client_app.wasm";
    auto client_buffer = readFileToBytes(client_wasm_path);
  
    // load module and create execution environment
    client_module = load_module_minimal(client_buffer, client_module_inst, client_exec_env, stack_size, heap_size, error_buf, sizeof(error_buf));
    if (!client_module) {
      return 1;
    }

    // ----------------

    std::string server_wasm_path = Config::get("WASM_OUT") + "/server_app.wasm";
    auto server_buffer = readFileToBytes(server_wasm_path);
  
    // load module and create execution environment
    server_module = load_module_minimal(server_buffer, server_module_inst, server_exec_env, stack_size, heap_size, error_buf, sizeof(error_buf));
    if (!server_module) {
      return 1;
    }

    // ---------------------------------------------------------------- client

    // calling client main function
    auto main_func = wasm_runtime_lookup_function(client_module_inst, "_start");
    if (!main_func) {
      fprintf(stderr, "_start wasm function is not found.\n");
      return 1;
    }

    pthread_t th;
    if (!start_wasm_thread(client_module_inst, main_func, &th)) {
      std::fprintf(stderr, "Thread spawn failed\n");
    }

    // ---------------------------------------------- server
    auto main_func2 = wasm_runtime_lookup_function(server_module_inst, "_start");
    if (!main_func2) {
      fprintf(stderr, "_start wasm function is not found.\n");
      return 1;
    }

    pthread_t th2;
    if (!start_wasm_thread(server_module_inst, main_func2, &th2)) {
      std::fprintf(stderr, "Thread spawn failed\n");
    }

    // ------------------------------
    // wait for branches
    pthread_join(th2, nullptr);  
    pthread_join(th, nullptr);  

    return 0;
}