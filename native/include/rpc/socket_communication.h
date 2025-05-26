#include <wasm_export.h>
#include <arpa/inet.h>

const int message_port = 12345;
const int response_port = 12346;
const in_addr_t default_ip = inet_addr("127.0.0.1");

void socket_listener(wasm_module_inst_t module_inst, int port = message_port, in_addr_t ip = default_ip);
void socket_response_listener(wasm_module_inst_t module_inst, int port = response_port, in_addr_t ip = default_ip);

void send_over_socket(const uint8_t* data, uint32_t length, const char* ip = "127.0.0.1", uint16_t port = message_port);
void send_response_over_socket(const uint8_t* data, uint32_t length, const char* ip = "127.0.0.1", uint16_t port = response_port);

