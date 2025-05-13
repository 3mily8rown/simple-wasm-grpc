#include <wasm_export.h>
#include <string.h>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <pthread.h>

#include "load_module.h"
#include "config.h"
#include "demangle.h"

#include "client_app_imports.h"
#include "server_app_imports.h"

#include "message.pb.h"

#define THREAD_STACK_SIZE (64 * 1024)

typedef struct {
  WASMModuleInstanceCommon *module_inst;
  wasm_function_inst_t       func;
} ThreadArg;

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

  static void* thread_main(void *arg) {
    ThreadArg *t = (ThreadArg*)arg;

    if (!wasm_runtime_init_thread_env()) {
      std::fprintf(stderr, "failed to init WAMR thread env\n");
      free(t);
      return nullptr;
    }
  
    // thread env
    WASMExecEnv *exec_env = wasm_runtime_create_exec_env(t->module_inst,
                                                         THREAD_STACK_SIZE);
    if (!exec_env) {
      fprintf(stderr, "failed to create exec env for thread\n");
      wasm_runtime_destroy_thread_env();
      free(t);
      return NULL;
    }
    
    // call wasm func
    if (!wasm_runtime_call_wasm(exec_env, t->func, 0, NULL)) {
      const char *ex = wasm_runtime_get_exception(t->module_inst);
      fprintf(stderr, "wasm thread failed: %s\n", ex ? ex : "(null)");
    }
  
    // clean everything
    wasm_runtime_destroy_exec_env(exec_env);
    wasm_runtime_destroy_thread_env();
    free(t);
    return NULL;
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

    // ----------------------------------------------------------------

    // calling client main function
    auto main_func = wasm_runtime_lookup_function(client_module_inst, "_start");
    if (!main_func) {
      fprintf(stderr, "_start wasm function is not found.\n");
      return 1;
    }

    ThreadArg *targ = static_cast<ThreadArg*>(std::malloc(sizeof(ThreadArg)));
    if (!targ) {
      std::fprintf(stderr, "out of memory\n");
      return 1;
    }
    targ->module_inst = client_module_inst;
    targ->func        = main_func;

    // ---------------------------------------------- now server
    auto main_func2 = wasm_runtime_lookup_function(server_module_inst, "_start");
    if (!main_func2) {
      fprintf(stderr, "_start wasm function is not found.\n");
      return 1;
    }

    ThreadArg *targ2 = static_cast<ThreadArg*>(std::malloc(sizeof(ThreadArg)));
    if (!targ2) {
      std::fprintf(stderr, "out of memory\n");
      return 1;
    }
    targ2->module_inst = server_module_inst;
    targ2->func        = main_func2;

    pthread_t tid2;
    int err2 = pthread_create(&tid2, NULL, thread_main, targ2);
    if (err2 != 0) {
      fprintf(stderr, "pthread_create failed: %s\n", strerror(err2));
      free(targ2);
      return 1;
    }
    // -----------------------------------------------


    pthread_t tid;
    int err = pthread_create(&tid, NULL, thread_main, targ);
    if (err != 0) {
      fprintf(stderr, "pthread_create failed: %s\n", strerror(err));
      free(targ);
      return 1;
    }

    // wait for branches
    pthread_join(tid, NULL);  
    pthread_join(tid2, NULL);  

    return 0;
}