#include <cstdio>
#include <cstring>

#include "wasm/call_wasm.h"
#include "wamr/thread_launch.h"

#define THREAD_STACK_SIZE (64 * 1024)

typedef struct {
    WASMModuleInstanceCommon *module_inst;
    wasm_function_inst_t       func;
} ThreadArg;

// Assumes func takes in no args, expects no returns
static void *wasm_thread_entry(void *raw_arg) {
    ThreadArg *arg = static_cast<ThreadArg*>(raw_arg);

    // thread env
    if (!wasm_runtime_init_thread_env()) {
        std::fprintf(stderr, "failed to init WAMR thread env\n");
        delete arg;
        return nullptr;
    }

    // thread exec env
    WASMExecEnv *exec_env =
        wasm_runtime_create_exec_env(arg->module_inst, THREAD_STACK_SIZE);
    if (!exec_env) {
        std::fprintf(stderr, "failed to create exec env for thread\n");
        wasm_runtime_destroy_thread_env();
        delete arg;
        return nullptr;
    }

    // call func
    if (!call_no_args(exec_env, arg->module_inst, arg->func)) {
        const char *ex = wasm_runtime_get_exception(arg->module_inst);
        std::fprintf(stderr,
                     "wasm thread failed: %s\n",
                     ex ? ex : "(null)");
    }

    // Clean up
    wasm_runtime_destroy_exec_env(exec_env);
    wasm_runtime_destroy_thread_env();
    delete arg;
    return nullptr;
}

bool start_wasm_thread(WASMModuleInstanceCommon *module_inst,
                       wasm_function_inst_t func,
                       pthread_t *out_thread) {

    ThreadArg *arg = new ThreadArg{ module_inst, func };
    if (!arg) {
        std::fprintf(stderr, "out of memory when allocating ThreadArg\n");
        return false;
    }

    pthread_t tid;
    int err = pthread_create(&tid, nullptr, wasm_thread_entry, arg);
    if (err != 0) {
        std::fprintf(stderr,
                     "pthread_create failed: %s\n",
                     strerror(err));
        delete arg;
        return false;
    }

    if (out_thread) {
        *out_thread = tid;
    } else {
        pthread_detach(tid);
    }

    return true;
}
