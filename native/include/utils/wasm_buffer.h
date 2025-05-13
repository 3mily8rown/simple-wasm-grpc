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

void write_into_buffer(const google::protobuf::Message& msg, wasm_module_inst_t module_inst, WasmBuffer buffer);

WasmBuffer make_wasm_buffer_and_write(const google::protobuf::Message& msg, wasm_module_inst_t module_inst);

WasmBuffer make_wasm_buffer(wasm_module_inst_t module_inst, uint32_t max_length);

WasmBuffer make_wasm_buffer(wasm_module_inst_t module_inst);

#endif 