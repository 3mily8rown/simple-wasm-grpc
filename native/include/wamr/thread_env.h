#pragma once
#include <wasm_export.h>
#include <stdexcept>

namespace wamr {

class ThreadEnv {
public:
    ThreadEnv() {
        if (!wasm_runtime_thread_env_inited()
            && !wasm_runtime_init_thread_env())
            throw std::runtime_error("TLS init failed");
    }
    ~ThreadEnv() { wasm_runtime_destroy_thread_env(); }
};

}
