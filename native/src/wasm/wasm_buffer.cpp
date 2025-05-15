#include <google/protobuf/message.h>
#include <string>
#include <cstring>

#include "wasm_export.h"  

struct WasmBuffer {
  uint32_t offset;
  uint32_t length;
};

WasmBuffer make_wasm_buffer(const google::protobuf::Message& msg, wasm_module_inst_t module_inst) {
    // Serialize
    std::string wire;
    if (!msg.SerializeToString(&wire)) {
    fprintf(stderr, "Protobuf serialize failed\n");
    return {0, 0};
    }

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