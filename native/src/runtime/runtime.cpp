#include "runtime.h"

#include <cstring>
#include <mutex>
#include <stdexcept>
#include <wasm_export.h> 

#include "native_symbols.h"

namespace wamr {

// **process‑wide singleton** – initialises WAMR once and destroys it at exit

Runtime::Runtime()
{
    std::lock_guard<std::mutex> guard(mu_);

    if (initialised_) return;

    //-------------------------------------------------- set‑up init arguments
    RuntimeInitArgs args;
    std::memset(&args, 0, sizeof(RuntimeInitArgs));

    args.mem_alloc_type                = Alloc_With_Pool;
    args.mem_alloc_option.pool.heap_buf = heap_;
    args.mem_alloc_option.pool.heap_size= sizeof(heap_);

    //-------------------------------------------------------------- init WAMR
    if (!wasm_runtime_full_init(&args))
        throw std::runtime_error("wamr::Runtime – wasm_runtime_full_init failed");

    // To reduce stack size for debugger - still get overflow
    // wasm_runtime_set_log_level(WASM_LOG_LEVEL_ERROR);

    register_all_env_symbols();

    initialised_ = true;
}

Runtime::~Runtime()
{
    std::lock_guard<std::mutex> guard(mu_);

    if (initialised_) {
        wasm_runtime_destroy();
        initialised_ = false;
    }
}

}
