#include <wasm_export.h>

#include "message.pb.h"

bool call_no_args(wasm_exec_env_t exec_env,
  wasm_module_inst_t inst,
  wasm_function_inst_t func) {
    printf("call no args\n"); // TODO remove 
    if (!func) {
        fprintf(stderr, "wasm function is not found.\n");
        return false;
    }
    if (!wasm_runtime_call_wasm(exec_env, func, 0, nullptr)) {
        const char* ex = wasm_runtime_get_exception(inst);
        fprintf(stderr, "wasm function trapped: %s\n", ex ? ex : "(null)");
        return false;
    }
    return true;
}
  
  
