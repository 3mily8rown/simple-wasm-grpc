#include <wasm_export.h>

void socket_listener(wasm_module_inst_t module_inst, int port = 12345);

void send_over_socket(const uint8_t* data, uint32_t length, const char* ip = "127.0.0.1", uint16_t port = 12345);
