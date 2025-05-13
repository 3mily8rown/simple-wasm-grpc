#include <wasm_export.h>
#include <platform_common.h>    

#include <cstring>
#include "load_module.h"
#include "config.h"
#include "client_app_imports.h"
#include "server_app_imports.h"

#include "wasm_context.h"

static constexpr uint32_t module_stack_size = 256 * 1024;
static constexpr uint32_t module_heap_size = 256 * 1024;
static const uint32_t module_error_buf_size = 128;
static const uint32_t max_threads = 2;

static constexpr uint32_t GLOBAL_HEAP_SIZE = 512 * 1024;


static NativeSymbol all_env_native_symbols[
    generated_client_app_native_symbols_count +
    generated_server_app_native_symbols_count];

static const size_t all_env_native_symbols_count =
generated_client_app_native_symbols_count +
generated_server_app_native_symbols_count;

void register_all_env_symbols() {
    size_t out_idx = 0;
    for (size_t i = 0; i < generated_client_app_native_symbols_count; ++i)
        all_env_native_symbols[out_idx++] = generated_client_app_native_symbols[i];
    for (size_t i = 0; i < generated_server_app_native_symbols_count; ++i)
        all_env_native_symbols[out_idx++] = generated_server_app_native_symbols[i];
    
    wasm_runtime_register_natives(
        "env",
        all_env_native_symbols,
        all_env_native_symbols_count
    );
}  

// Need to wrap malloc/free/realloc since cpp returns size_t
// extern "C" {
//     static void* wasm_host_malloc(uint32 size) noexcept { return std::malloc(size); }
//     static void* wasm_host_realloc(void* p, uint32 size) noexcept { return std::realloc(p, size); }
//     static void  wasm_host_free(void* p) noexcept { std::free(p); }
// }
  
WasmContext initialize_wasm() {
    WasmContext ctx{};
    ctx.ok = false;

    ctx.dimensions.stack_size = module_stack_size;
    ctx.dimensions.heap_size = module_heap_size;
    ctx.dimensions.error_buf_size = module_error_buf_size;
    ctx.dimensions.max_threads = max_threads;
    // TODO if keep pool add global heap size back

    wasm_runtime_set_log_level(WASM_LOG_LEVEL_DEBUG);

    RuntimeInitArgs init_args;
    // static char global_heap_buf[GLOBAL_HEAP_SIZE];
    memset(&init_args, 0, sizeof(init_args));
    init_args.mem_alloc_type = Alloc_With_System_Allocator;
    // init_args.mem_alloc_option.allocator.malloc_func = reinterpret_cast<void*>(wasm_host_malloc);
    // init_args.mem_alloc_option.allocator.realloc_func = reinterpret_cast<void*>(wasm_host_realloc);
    // init_args.mem_alloc_option.allocator.free_func = reinterpret_cast<void*>(wasm_host_free);
    // init_args.mem_alloc_option.allocator.user_data = nullptr;

    // init_args.mem_alloc_type = Alloc_With_Pool;
    // init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    // init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);

    init_args.max_thread_num = max_threads;

    if (!wasm_runtime_full_init(&init_args)) {
        ctx.error = "wasm_runtime_full_init failed";
        return ctx;
    }

    wasm_runtime_set_log_level(WASM_LOG_LEVEL_VERBOSE);

    register_all_env_symbols();

    //load wasm module helper
    auto load_one = [&](std::vector<uint8_t>& buffer,
                        wasm_module_t& out_mod,
                        wasm_module_inst_t& out_inst,
                        wasm_exec_env_t& out_env) -> bool {
        char  errbuf[module_error_buf_size];

        out_mod = load_module_minimal(
                    buffer.data(),
                    uint32_t(buffer.size()),
                    out_inst,
                    out_env,
                    module_stack_size,
                    module_heap_size,
                    errbuf,
                    sizeof(errbuf));
        if (!out_mod) {
            ctx.error = std::string("Failed to load ") + ": " + errbuf;
            return false;
        }
        if (!out_inst) {
            ctx.error = std::string("Instantiating failed: ") + errbuf;
            return false;
          }
        
          if (!out_env) {
            ctx.error = "Creating exec_env failed";
            return false;
          }
        return true;
    };

    // load modules
    std::string base = Config::get("WASM_OUT") + "/";
    ctx.client_bytes = readFileToBytes(base + "client_app.wasm");
    ctx.server_bytes = readFileToBytes(base + "server_app.wasm");

    if (!load_one(ctx.client_bytes,
                    ctx.client_module,
                    ctx.client_module_inst,
                    ctx.client_exec_env))
        return ctx;

    if (!load_one(ctx.server_bytes,
                    ctx.server_module,
                    ctx.server_module_inst,
                    ctx.server_exec_env))
        return ctx;

    ctx.ok = true;
    return ctx;
}
