#include <wasm_export.h>
#include <string.h>
#include <stdio.h>
#include <string>

#include "load_module.h"
#include "config.h"
#include "demangle.h"

#include "client_app_imports.h"
#include "client_app_exports.h"

#include "message.pb.h"

void pass_to_native_grpc(wasm_exec_env_t exec_env, uint32_t offset, uint32_t length);

void register_env_symbols() {
    wasm_runtime_register_natives(
      "env",
      generated_client_app_native_symbols,
      generated_client_app_native_symbols_count
    );
  }

int main() {
    // setting up wasm module
    wasm_runtime_set_log_level(WASM_LOG_LEVEL_DEBUG);
  
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
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
  
    register_env_symbols();
  
    std::string lol = Config::get("ROOT_DIR");
    printf("%s\n", lol.c_str());
    std::string wasmPath = Config::get("WASM_OUT") + "/client_app.wasm";
    auto buffer = readFileToBytes(wasmPath);
  
    // load module and create execution environment
    module = load_module_minimal(buffer, module_inst, exec_env, stack_size, heap_size, error_buf, sizeof(error_buf));
    if (!module) {
      return 1;
    }
  
    cache_all_exports(module_inst, CLIENT_APP_EXPORT_NAMES);

    // sending a message from wasm to native using protobuffers
    auto func1 = get_exported_func("send_message", module_inst);
    if (!func1) {
      fprintf(stderr, "send_message wasm function is not found.\n");
      return 1;
    }
    
    if (!wasm_runtime_call_wasm(exec_env, func1, 0, nullptr)) {
      const char* ex = wasm_runtime_get_exception(module_inst);
      fprintf(stderr, "send_message trapped: %s\n", ex ? ex : "(null)");
    }

    // calling wasm main function
    auto main = get_exported_func("_start", module_inst);
    if (!main) {
      fprintf(stderr, "main wasm function is not found.\n");
      return 1;
    }
    
    if (!wasm_runtime_call_wasm(exec_env, main, 0, nullptr)) {
      const char* ex = wasm_runtime_get_exception(module_inst);
      fprintf(stderr, "main wasm failed: %s\n", ex ? ex : "(null)");
    }

    return 0;
}