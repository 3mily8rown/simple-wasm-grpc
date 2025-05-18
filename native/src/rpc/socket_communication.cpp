#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include "rpc/message_queue.h"
#include "rpc/socket_communication.h"


void socket_listener(wasm_module_inst_t module_inst, int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr = { AF_INET, htons(port), INADDR_ANY };
    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 1);

    std::cout << "Server host listening...\n";
    int client_fd = accept(server_fd, nullptr, nullptr);

    uint8_t buffer[1024];
    ssize_t n = read(client_fd, buffer, sizeof(buffer));
    if (n > 0) {
        queue_message(buffer, n);
    }

    close(client_fd);
    close(server_fd);
}

void send_over_socket(const uint8_t* data, uint32_t length, const char* ip, uint16_t port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[Client] socket");
        return;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[Client] connect");
        std::fprintf(stderr, "[Client] Dropping undeliverable message (%u bytes)\n", length);
        close(sock);
        return;
    }

    ssize_t sent = send(sock, data, length, 0);
    if (sent < 0) {
        perror("[Client] send");
    } else if (static_cast<uint32_t>(sent) != length) {
        std::fprintf(stderr, "[Client] Partial send: %zd of %u bytes\n", sent, length);
    }

    close(sock);
}
