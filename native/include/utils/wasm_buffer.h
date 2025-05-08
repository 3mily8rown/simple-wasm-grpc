// wasm_buffer.h
#ifndef WASM_BUFFER_H
#define WASM_BUFFER_H

#include <cstdint>
#include <google/protobuf/message.h>
#include <wasm_export.h>  

/// A simple pair of (offset,length) into WASM linear memory
struct WasmBuffer {
  uint32_t offset;
  uint32_t length;
};

WasmBuffer make_wasm_buffer(const google::protobuf::Message& msg,
                            wasm_module_inst_t module_inst);

#endif 