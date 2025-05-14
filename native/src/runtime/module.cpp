#include "module.h"
#include "thread_env.h"

#include <cstring>
#include <stdexcept>
#include <wasm_export.h>

namespace wamr {

    static std::string lastError(const char* buf)
    {
        return std::string("WAMR: ") + (buf ? buf : "(no detail)");
    }

    // **per‑WASM‑module RAII wrapper**
    Module::Module(std::vector<uint8_t>& bytes,
        uint32_t              stack_kb,
        uint32_t              heap_kb)
    {
    char err[128];

    module_ = wasm_runtime_load(bytes.data(), bytes.size(), err, sizeof err);
    if (!module_) throw std::runtime_error(lastError(err));

    inst_ = wasm_runtime_instantiate(module_,
                                stack_kb,
                                heap_kb,
                                err, sizeof err);
    if (!inst_) throw std::runtime_error(lastError(err));

    exec_ = wasm_runtime_create_exec_env(inst_, stack_kb);
    if (!exec_) throw std::runtime_error("exec_env creation failed");

    }


    Module::~Module()
    {
        if (exec_)
            wasm_runtime_destroy_exec_env(exec_);
        if (inst_)
            wasm_runtime_deinstantiate(inst_);
        if (module_)
            wasm_runtime_unload(module_);
    }

} // namespace wamr
