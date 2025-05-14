#include "client_app_imports.h"
#include "server_app_imports.h"

static const size_t all_env_native_symbols_count =
generated_client_app_native_symbols_count +
generated_server_app_native_symbols_count;

static NativeSymbol all_env_native_symbols[all_env_native_symbols_count];

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
