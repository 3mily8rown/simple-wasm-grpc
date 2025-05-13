#include <google/protobuf/message.h>
#include <string>
#include <cstring>

#include "wasm_export.h"  

// NOT USED - but could easily if make choice to not send receive message with a buffer already
struct WasmBuffer {
  uint32_t offset;
  uint32_t length;
};

void write_into_buffer(const google::protobuf::Message& msg, wasm_module_inst_t module_inst, WasmBuffer buffer) {
  // Find buffer address to write into
  uint8_t* memory = static_cast<uint8_t*>(wasm_runtime_addr_app_to_native(module_inst, buffer.offset));
  if (!memory) {
    printf("Invalid memory address\n");
    return;
  }
  
  // Serialize
  std::string wire;
  if (!msg.SerializeToString(&wire)) {
    fprintf(stderr, "Protobuf serialize failed\n");
    return;
  }

  memcpy(memory, wire.data(), wire.size());
}

WasmBuffer make_wasm_buffer(wasm_module_inst_t module_inst, uint32_t max_length) {
  // Allocate in Wasm linear heap
  void* host_ptr = nullptr;
  uint32_t offset = wasm_runtime_module_malloc(module_inst, max_length, &host_ptr);
  if (offset == 0 || host_ptr == nullptr) {
      fprintf(stderr, "wasm malloc failed\n");
      return {0, 0};
  }
  return { offset, max_length };
}

WasmBuffer make_wasm_buffer(wasm_module_inst_t module_inst) {
  return make_wasm_buffer(module_inst, 128);
}

WasmBuffer make_wasm_buffer_and_write(const google::protobuf::Message& msg, wasm_module_inst_t module_inst) {
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