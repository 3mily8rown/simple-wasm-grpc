#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include <iostream>
#include <cstring>

#include <stdint.h>
#include <stdlib.h>
#include <chrono>
#include <thread>


#include "pb_encode.h"
#include "pb_decode.h"
#include "message.pb.h"
#include "rpc_envelope.pb.h"



class RpcClient {
    int sockfd;

public:
    RpcClient(const char* server_ip, int port) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) perror("socket");

        sockaddr_in serv_addr{};
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) != 1) {
            perror("inet_pton");
            throw std::runtime_error(std::string("Failed to resolve hostname: ") + server_ip);
        }



        for (int i = 0; i < 10; ++i) {
        if (connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) == 0) {
            break;  // success
        }
        perror("connect");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (i == 9) {
            std::cerr << "Failed to connect after 10 attempts\n";
            exit(1);
        }
}

    }

    ~RpcClient() {
        close(sockfd);
    }

    void Send(const char* method, const void* message, const pb_msgdesc_t* fields) {
        // Encode payload
        uint8_t payload_buf[128];
        pb_ostream_t payload_stream = pb_ostream_from_buffer(payload_buf, sizeof(payload_buf));
        if (!pb_encode(&payload_stream, fields, message)) {
            printf("Failed to encode payload: %s\n", PB_GET_ERROR(&payload_stream));
            return;
        }

        // Create envelope
        RpcEnvelope env = RpcEnvelope_init_zero;
        strncpy(env.method, method, sizeof(env.method) - 1);
        memcpy(env.payload.bytes, payload_buf, payload_stream.bytes_written);
        env.payload.size = payload_stream.bytes_written;

        // Encode envelope
        uint8_t envelope_buf[256];
        pb_ostream_t env_stream = pb_ostream_from_buffer(envelope_buf, sizeof(envelope_buf));
        if (!pb_encode(&env_stream, RpcEnvelope_fields, &env)) {
            printf("Failed to encode RpcEnvelope: %s\n", PB_GET_ERROR(&env_stream));
            return;
        }

        // Send envelope size (optional, for framing)
        uint32_t len = env_stream.bytes_written;
        send(sockfd, &len, sizeof(len), 0);  // send length prefix

        // Send envelope
        send(sockfd, envelope_buf, len, 0);
    }

    Ack ReceiveAck() {
        uint32_t len;
        int r = recv(sockfd, &len, sizeof(len), MSG_WAITALL);
        if (r <= 0) {
            printf("No response received (length)\n");
            return Ack_init_zero;
        }

        uint8_t buf[256];
        r = recv(sockfd, buf, len, MSG_WAITALL);
        if (r <= 0) {
            printf("No response received (payload)\n");
            return Ack_init_zero;
        }

        RpcResponse resp = RpcResponse_init_zero;
        pb_istream_t stream = pb_istream_from_buffer(buf, len);
        if (!pb_decode(&stream, RpcResponse_fields, &resp)) {
            printf("Failed to decode RpcResponse: %s\n", PB_GET_ERROR(&stream));
            return Ack_init_zero;
        }

        printf("[Client] Received RpcResponse (id=%u, status=%s)\n", resp.request_id, resp.status);

        Ack ack = Ack_init_zero;
        pb_istream_t ack_stream = pb_istream_from_buffer(resp.payload.bytes, resp.payload.size);
        if (!pb_decode(&ack_stream, Ack_fields, &ack)) {
            printf("Failed to decode Ack: %s\n", PB_GET_ERROR(&ack_stream));
            return Ack_init_zero;
        }

        return ack;
    }
};

inline const char* get_server_ip() {
    const char* ip = getenv("SERVER_HOST");
    return ip ? ip : "127.0.0.1";  // fallback for non-Docker environments
}

int main() {
    printf("Client main function\n");

    // Connect to server on localhost:12345
    RpcClient client(get_server_ip(), 12345);

    // Construct message
    MyMessage msg = MyMessage_init_default;
    msg.id = 42;
    strcpy(msg.name, "hello from client");

    // Start timer
    auto start = std::chrono::high_resolution_clock::now();

    // Send request
    client.Send("SendMessage", &msg, MyMessage_fields);

    // Receive and print ack
    Ack ack = client.ReceiveAck();

    // End timer
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    printf("[Client] Ack: success=%d, info=\"%s\"\n", ack.success, ack.info);
    printf("[METRICS] Round-trip time: %ld μs\n", duration_us);

    return 0;
}


