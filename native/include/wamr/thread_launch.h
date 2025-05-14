#pragma once
#include <pthread.h>
#include <wasm_export.h>

namespace wamr {

/// Spawn a POSIX thread that calls `func` (no args / void) on a new WAMR
/// execution environment cloned from `parent_exec_env`.
bool start_wasm_thread(wasm_exec_env_t      parent_exec_env,
                       wasm_function_inst_t func,
                       pthread_t*           out_thread = nullptr);

} // namespace wamr
