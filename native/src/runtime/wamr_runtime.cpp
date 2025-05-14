// // ---------- global singleton ------------------------------------------------
// #include <mutex>
// #include <vector>
// #include <wasm_export.h>
// #include <stdexcept>

// #include "native_symbols.h"

// class WamrRuntime {
//     public:
//         WamrRuntime() {
//             std::lock_guard<std::mutex> g(mu_);
//             if (initialised_) return;
    
//             RuntimeInitArgs args{};
//             args.mem_alloc_type = Alloc_With_Pool;
//             args.mem_alloc_option.pool.heap_buf  = globalHeap_;
//             args.mem_alloc_option.pool.heap_size = sizeof(globalHeap_);
//             if (!wasm_runtime_full_init(&args))
//                 throw std::runtime_error("WAMR init failed");
    
//             register_all_env_symbols();
//             initialised_ = true;
//         }
//         ~WamrRuntime() { wasm_runtime_destroy(); }
    
//     private:
//         inline static bool          initialised_ = false;
//         inline static std::mutex    mu_;
//         inline static char          globalHeap_[512 * 1024];
// };
    
// // ---------- per‑thread guard -------------------------------------------------
// class WamrThreadEnv {
// public:
//     WamrThreadEnv() {
//         if (!wasm_runtime_thread_env_inited()
//             && !wasm_runtime_init_thread_env())
//             throw std::runtime_error("TLS init failed");
//     }
//     ~WamrThreadEnv() { wasm_runtime_destroy_thread_env(); }
// };
    
// // ---------- module wrapper ---------------------------------------------------
// class WasmModule {
// public:
//     WasmModule(std::vector<uint8_t>& bytes,
//                 uint32_t stack = 8 * 1024,
//                 uint32_t heap  = 8 * 1024)
//     {
//         // Ensure global runtime is ready
//         static WamrRuntime runtime;

//         char err[128];
//         module_ = wasm_runtime_load(bytes.data(), bytes.size(),
//                                     err, sizeof err);
//         if (!module_) throw std::runtime_error(err);

//         inst_   = wasm_runtime_instantiate(module_, stack, heap, err, sizeof err);
//         if (!inst_) throw std::runtime_error(err);

//         exec_   = wasm_runtime_create_exec_env(inst_, stack);
//         if (!exec_) throw std::runtime_error("exec_env failed");
//     }

//     ~WasmModule() {
//         if (exec_)   wasm_runtime_destroy_exec_env(exec_);
//         if (inst_)   wasm_runtime_deinstantiate(inst_);
//         if (module_) wasm_runtime_unload(module_);
//     }

//     wasm_exec_env_t exec()   const { return exec_; }
//     wasm_module_inst_t inst() const { return inst_; }

// private:
//     wasm_module_t       module_ = nullptr;
//     wasm_module_inst_t  inst_   = nullptr;
//     wasm_exec_env_t     exec_   = nullptr;
// };
