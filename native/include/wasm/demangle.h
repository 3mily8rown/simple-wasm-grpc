#include "wasm_export.h"
#include <string>
#include <unordered_map>

struct WasmExport {
    const char *pretty;
    const char *mangled;
};

// Build the cache of exports for a
void cache_all_exports(wasm_module_inst_t module_inst,
                       const WasmExport export_names[]);

// Get a cached export by it's demangled name
// WASMFunctionInstanceCommon* get_exported_func(const std::string& name, 
//                                             wasm_module_inst_t module_inst,
//                                             const std::vector<wasm_val_t>& args);

WASMFunctionInstanceCommon* get_exported_func(const std::string& name, 
    wasm_module_inst_t module_inst,
    const wasm_val_t args[],
    size_t count);

WASMFunctionInstanceCommon* get_exported_int_func(const std::string& name, 
    wasm_module_inst_t module_inst,
    const size_t len);

WASMFunctionInstanceCommon* get_exported_func(const std::string& name, 
    wasm_module_inst_t module_inst );

std::string buildTypedKey(const std::string& name, const wasm_val_t args[], size_t count);
std::string buildIntTypedKey(const std::string& name, size_t len);

