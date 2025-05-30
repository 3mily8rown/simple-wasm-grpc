#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>

#include "rpc/message_queue.h"
#include "rpc/socket_communication.h"

static constexpr int MAX_CONNECT_ATTEMPTS = 5;
static constexpr int MAX_SEND_ATTEMPTS    = 3;
static constexpr int INITIAL_BACKOFF_MS   = 100;

in_addr_t resolve_ip_or_throw(const char* hostname);


void socket_listener(int port, in_addr_t ip) {
    if (ip == INADDR_NONE) {
        ip = resolve_ip_or_throw(get_server_ip());  // or get_client_ip()
    }


    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr = { AF_INET, htons(port), ip };
    char ip_buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip, ip_buf, sizeof(ip_buf));
    std::cout << "[Native] Binding to " << ip_buf << ":" << port << "\n";


    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 1);

    std::cout << "Host listening over port " << port << "...\n";

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        while (true) {
            uint8_t buffer[1024];
            ssize_t n = read(client_fd, buffer, sizeof(buffer));
            if (n <= 0) break; // client closed or error

            std::cout << "[Native] Received " << n << " bytes on port " << port << "\n";

            if (port == message_port) {
                queue_message(buffer, n);
                std::cout << "[Native] Queued message for server\n";
            } else if (port == response_port) {
                queue_response(buffer, n);
                std::cout << "[Native] Queued response for client\n";
            } else {
                std::cerr << "[Native] Unknown port: " << port << "\n";
            }
        }

        close(client_fd); // clean up when client closes
    }
    close(server_fd);
}

void socket_response_listener(int port, in_addr_t ip) {
    if (ip == INADDR_NONE) {
        ip = resolve_ip_or_throw(get_client_ip());  // or get_client_ip()
    }

    socket_listener(port, ip);
}

void send_over_socket(const uint8_t* data, uint32_t length, const char* ip, uint16_t port) {
    std::cout << "[Native] Sending to " << ip << ":" << port << "\n";

    std::string tag = (port == message_port) ? "[Client] " : "[Server] ";
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror((tag + " socket").c_str());
        return;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) != 1) {
        perror((tag + " inet_pton").c_str());
        close(sock);
        return;
    }



    std::cout << "[Native] Attempting to connect to " << ip << ":" << port << "\n";
    int result = connect(sock, (sockaddr*)&server_addr, sizeof(server_addr));
    std::cout << "[Native] connect() returned " << result << "\n";

    if (result < 0) {
        perror((tag + " connect").c_str());
        std::fprintf(stderr, "%s Dropping undeliverable message (%u bytes)\n", tag.c_str(), length);
        close(sock);
        return;
    }

    ssize_t sent = send(sock, data, length, 0);
    if (sent < 0) {
        perror((tag + " send").c_str());
    } else if (static_cast<uint32_t>(sent) != length) {
        std::fprintf(stderr, "%s Partial send: %zd of %u bytes\n", tag.c_str(), sent, length);
    }

    close(sock);
}

void send_response_over_socket(const uint8_t* data, uint32_t length, const char* ip, uint16_t port) {
    std::cout << "[Native] Sending response to " << ip << ":" << port << "\n";
    send_over_socket(data, length, ip, port);
}

void send_over_socket(const uint8_t* data, uint32_t length) {
    send_over_socket(data, length, get_client_ip(), message_port);
}

void send_response_over_socket(const uint8_t* data, uint32_t length) {
    send_response_over_socket(data, length, get_server_ip(), response_port);
}

in_addr_t resolve_ip_or_throw(const char* hostname) {
    in_addr addr_parsed{};
    if (inet_pton(AF_INET, hostname, &addr_parsed) == 1) {
        return addr_parsed.s_addr;
    }

    hostent* host = gethostbyname(hostname);
    if (!host) {
        throw std::runtime_error(std::string("Failed to resolve hostname: ") + hostname);
    }

    in_addr_t ip;
    std::memcpy(&ip, host->h_addr, host->h_length);
    return ip;
}

