#pragma once
#include "runtime.h"
#include <vector>
#include <wasm_export.h>

namespace wamr {

class Module {
public:
    Module(std::vector<uint8_t>& bytes,
           uint32_t stack_kb = 8 * 1024,
           uint32_t heap_kb  = 8 * 1024);
    ~Module();

    wasm_module_inst_t inst() const { return inst_; }
    wasm_exec_env_t    exec() const { return exec_; }

private:
    Runtime             ensureGlobal_;
    wasm_module_t       module_ = nullptr;
    wasm_module_inst_t  inst_   = nullptr;
    wasm_exec_env_t     exec_   = nullptr;
};

}
