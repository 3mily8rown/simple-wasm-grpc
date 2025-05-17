#include <string>
#include <cstring>

#include "wasm_export.h"  
#include "wasm/wasm_buffer.h"  

// assumes message is serialised to string, but if bytes or something just swap that in the param
WasmBuffer make_wasm_buffer(const std::string wire, wasm_module_inst_t module_inst) {
    // Allocate in Wasm linear heap
    void* host_ptr = nullptr;
    uint32_t offset = wasm_runtime_module_malloc(module_inst, wire.size(), &host_ptr);
    if (offset == 0 || host_ptr == nullptr) {
        fprintf(stderr, "wasm malloc failed\n");
        return {0, 0};
    }

    // Copy bytes into Wasm memory
    memcpy(host_ptr, wire.data(), wire.size());

    return { offset, static_cast<uint32_t>(wire.size()) };
}