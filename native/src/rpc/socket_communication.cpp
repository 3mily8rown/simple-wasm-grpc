#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include <chrono>
#include <thread>

// #include "rpc/message_queue.h"
#include "ring_buffer_rpc/rpc_messaging.h"
#include "rpc/socket_communication.h"

static constexpr int MAX_CONNECT_ATTEMPTS = 5;
static constexpr int MAX_SEND_ATTEMPTS    = 3;
static constexpr int INITIAL_BACKOFF_MS   = 100;

in_addr_t resolve_ip_or_throw(const char* hostname);


void socket_listener(int port, in_addr_t ip) {
    // 1) Create socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("[Native] socket");
        exit(1);
    }

    // 2) Bind to ALL interfaces
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);            // <-- key change

    char ip_buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip_buf, sizeof(ip_buf));
    std::cout << "[Native] Binding to " << ip_buf << ":" << port << "\n";

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("[Native] bind");
        exit(1);
    }

    if (listen(server_fd, 1) < 0) {
        perror("[Native] listen");
        exit(1);
    }
    // std::cout << "[Native] Host listening on port " << port << "...\n";

    // 3) Accept loop
    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            perror("[Native] accept");
            continue;
        }

        // 4) Read loop
        uint8_t buffer[1024];
        ssize_t n;
        while ((n = read(client_fd, buffer, sizeof(buffer))) > 0) {
            std::cout << "[Native] Received " << n << " bytes on port " << port << "\n";
            if (port == message_port) {
                queue_message(buffer, n);
                // std::cout << "[Native] Queued message for server\n";
            } else if (port == response_port) {
                // todo fix this
                deliver_response(0,buffer, n);
                // std::cout << "[Native] Queued response for client\n";
            } else {
                std::cerr << "[Native] Unknown port: " << port << "\n";
            }
        }

        if (n < 0) {
            perror("[Native] read");
        }
        close(client_fd);
    }

    // never actually reached
    close(server_fd);
}


void socket_response_listener(int port, in_addr_t ip) {
    if (ip == INADDR_NONE) {
        ip = resolve_ip_or_throw(get_client_ip());  // or get_client_ip()
    }

    socket_listener(port, ip);
}

bool try_connect_with_retry(int sock,
                            const sockaddr_in* addr,
                            socklen_t addrlen,
                            const std::string& tag) {
    int attempt = 0;
    int backoff = INITIAL_BACKOFF_MS;
    while (attempt < MAX_CONNECT_ATTEMPTS) {
        int result = connect(sock, (const sockaddr*)addr, addrlen);
        if (result == 0) {
            return true;  // connected!
        }
        ++attempt;
        std::cerr << tag
                  << " connect attempt " << attempt
                  << " failed (errno=" << errno << "), retrying in "
                  << backoff << "ms...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(backoff));
        backoff *= 2;
    }
    return false;  // all attempts failed
}

void send_over_socket(const uint8_t* data, uint32_t length, const char* ip, uint16_t port) {
    // std::cout << "[Native] Sending to " << ip << ":" << port << "\n";

    std::string tag = (port == message_port) ? "[Client] " : "[Server] ";
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror((tag + " socket").c_str());
        return;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    try {
        server_addr.sin_addr.s_addr = resolve_ip_or_throw(ip);
    } catch (const std::exception& e) {
        std::cerr << "[Native] Failed to resolve '" << ip << "': "
                  << e.what() << "\n";
        close(sock);
        return;
    }



    // std::cout << tag << " [Native] Attempting to connect to " << ip << ":" << port << "\n";
    bool result = try_connect_with_retry(sock, &server_addr, sizeof(server_addr), tag);
    // std::cout << "[Native] connect() returned " << result << "\n";

    // if (result < 0) {
    if (!result) {
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
    send_over_socket(data, length, get_server_ip(), message_port);
}

void send_response_over_socket(const uint8_t* data, uint32_t length) {
    send_response_over_socket(data, length, get_client_ip(), response_port);
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

