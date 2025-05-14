#include "wamr/thread_env.h"
#include "wamr/thread_launch.h"

#include <cstdio>
#include <cstring>
#include <memory>

namespace wamr {

namespace {

struct ThreadArg {
    wasm_exec_env_t     parent_env;
    wasm_function_inst_t func;
};

// deleter for spawned exec‑envs
struct SpawnedEnvDeleter {
    void operator()(WASMExecEnv* e) const {
        if (e) wasm_runtime_destroy_spawned_exec_env(e);
    }
};

static void* wasm_thread_entry(void* raw_arg)
{
    std::unique_ptr<ThreadArg> arg{static_cast<ThreadArg*>(raw_arg)};

    ThreadEnv tls;   // make sure this POSIX thread has WAMR TLS

    // clone the parent's exec‑env; WAMR stores it in TLS for us
    std::unique_ptr<WASMExecEnv, SpawnedEnvDeleter> exec_env{
        wasm_runtime_spawn_exec_env(arg->parent_env)};
    if (!exec_env) {
        std::fprintf(stderr, "spawn exec env failed\n");
        return nullptr;
    }

    uint32_t dummy = 0;
    if (!wasm_runtime_call_wasm(exec_env.get(), arg->func, 0, &dummy)) {
        const char* ex = wasm_runtime_get_exception(
            wasm_runtime_get_module_inst(exec_env.get()));
        std::fprintf(stderr, "wasm thread failed: %s\n", ex ? ex : "(null)");
    }
    return nullptr;
}

} // anonymous namespace


bool start_wasm_thread(wasm_exec_env_t      parent_exec_env,
                       wasm_function_inst_t func,
                       pthread_t*           out_thread)
{
    if (!parent_exec_env) {
        std::fprintf(stderr,
                     "start_wasm_thread: parent exec_env is null\n");
        return false;
    }

    auto* arg = new (std::nothrow) ThreadArg{parent_exec_env, func};
    if (!arg) {
        std::fprintf(stderr, "out of memory allocating ThreadArg\n");
        return false;
    }

    pthread_t tid;
    int err = pthread_create(&tid, nullptr, wasm_thread_entry, arg);
    if (err != 0) {
        std::fprintf(stderr, "pthread_create failed: %s\n", strerror(err));
        delete arg;
        return false;
    }

    if (out_thread) *out_thread = tid;
    else            pthread_detach(tid);
    return true;
}

} // namespace wamr
