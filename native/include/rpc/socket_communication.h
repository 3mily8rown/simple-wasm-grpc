#include <wasm_export.h>
#include <arpa/inet.h>

// Ports
constexpr int message_port = 12345;
constexpr int response_port = 12346;

// Default IP strings
constexpr const char* default_server_ip = "client_container";
constexpr const char* default_client_ip = "server_container";

void socket_listener(wasm_module_inst_t module_inst, int port = message_port, in_addr_t ip = INADDR_NONE);
void socket_response_listener(wasm_module_inst_t module_inst, int port = response_port, in_addr_t ip = INADDR_NONE);

void send_over_socket(const uint8_t* data, uint32_t length, const char* ip = default_client_ip, uint16_t port = message_port);
void send_response_over_socket(const uint8_t* data, uint32_t length, const char* ip = default_server_ip, uint16_t port = response_port);

