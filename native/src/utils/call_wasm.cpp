#include <cstdio>
#include <string>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wasm_export.h>

#include "message.pb.h"
#include "demangle.h"

// Call a cached export function that takes in vector of int32 args and outputs 1 int32
// Returns 0 on success, non-zero on error.
int call_cached_int_func(
    wasm_module_inst_t module_inst,
    wasm_exec_env_t exec_env,
    const std::string &name,
    const std::vector<uint32_t>& args,
    int32_t& result_out) {

    // Convert uint32_t args to wasm_val_t
    std::vector<wasm_val_t> wasm_args;
    wasm_args.reserve(args.size());
    for (auto a : args) {
      wasm_val_t v;
      v.kind = WASM_I32;
      v.of.i32 = a;
      wasm_args.push_back(v);
    }

    auto func = get_exported_func(name, module_inst, wasm_args.data(), wasm_args.size());
    if (!func) {
      fprintf(stderr, "null function pointer\n");
      return 1;
    }
  
    // result
    wasm_val_t results[1];
    results[0].kind = WASM_I32;
    results[0].of.i32 = 0;
  
    // call func
    if (!wasm_runtime_call_wasm_a(exec_env,func, 1, results, wasm_args.size(), wasm_args.data())) {
        printf("call wasm function %s failed. %s\n",
        func,
        wasm_runtime_get_exception(module_inst));
        return 1;
    }
  
    result_out = results[0].of.i32;
    return 0;
}
  
  
