#include <cxxabi.h>
#include <unordered_map>
#include <mutex>
#include <cstdio>
#include <wasm_export.h>
#include <string>
#include <vector>
#include <sstream>
#include <cstdint>

#include "demangle.h"

// CURRENTLY NOT USED
// to use need to uncomment python generate exports section of build_wasm.sh

using ExportMap = std::unordered_map<std::string, WASMFunctionInstanceCommon*>;

static std::unordered_map<wasm_module_inst_t, ExportMap>  exports_map;
static std::mutex map_mutex;

// if want just one cache for all modules could just have the Inner map and not do m.clear()
void cache_all_exports(wasm_module_inst_t inst,
                      const WasmExport exports[]) {
  std::lock_guard<std::mutex> guard(map_mutex);
  auto &map = exports_map[inst]; 
  map.clear();

  for (auto *e = exports; e->pretty; ++e) {
    if (auto *fn = wasm_runtime_lookup_function(inst, e->mangled)) {
      map.emplace(e->pretty, fn);
    }
  }
}

// WASMFunctionInstanceCommon* get_exported_func(const std::string& name, wasm_module_inst_t module_inst, const std::vector<wasm_val_t>& args) {
//   return get_exported_func(buildTypedKey(name, args), module_inst);
// }

WASMFunctionInstanceCommon* get_exported_func(const std::string& name, wasm_module_inst_t module_inst, const wasm_val_t args[], size_t count) {
  return get_exported_func(buildTypedKey(name, args, count), module_inst);
}

WASMFunctionInstanceCommon* get_exported_int_func(const std::string& name, wasm_module_inst_t module_inst, const size_t len) {
  return get_exported_func(buildIntTypedKey(name, len), module_inst);
}

WASMFunctionInstanceCommon* get_exported_func(const std::string& typed_name, wasm_module_inst_t module_inst) {
  std::lock_guard<std::mutex> guard(map_mutex);

  // find module map
  auto mod = exports_map.find(module_inst);
  if (mod == exports_map.end()) {
    std::fprintf(stderr,
                 "ERROR: module_inst %p not cached\n", (void*)module_inst);
    return nullptr;
  }

  // find function
  auto &map = mod->second;
  auto func = map.find(typed_name);
  if (func == map.end()) {
    std::fprintf(stderr,
                 "ERROR: export '%s' not found in module %p\n",
                 typed_name.c_str(), (void*)module_inst);
    return nullptr;
  }
  return func->second;
}

std::string normalize_kind(uint8_t kind) {
  switch (kind) {
      case WASM_I32: return "int";
      case WASM_I64: return "int64";
      case WASM_F32: return "float";
      case WASM_F64: return "double";
      default:       return "unknown";
  }
}

std::string buildTypedKey(const std::string& name, const wasm_val_t args[], size_t count) {
  if (count == 0) {
      return name;
  }

  std::ostringstream oss;
  oss << name << ":";
  for (size_t i = 0; i < count; ++i) {
      if (i > 0) oss << ",";
      oss << normalize_kind(args[i].kind);
  }
  return oss.str();
}

std::string buildIntTypedKey(const std::string& name, size_t length) {
  if (length == 0) {
      return name;
  }

  std::ostringstream oss;
  oss << name << ":";
  for (size_t i = 0; i < length; ++i) {
      if (i > 0) oss << ",";
      oss << "int";
  }
  return oss.str();
}