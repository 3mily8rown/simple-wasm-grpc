#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/socket.h>    // socket, connect
#include <netinet/in.h>    // sockaddr_in, htons
#include <arpa/inet.h>     // inet_pton
#include <unistd.h>        // close
#include <string.h>        // memset, strncpy
#include <stdio.h>         // printf


#include "pb_encode.h"
#include "pb_decode.h"
#include "message.pb.h"
#include "rpc_envelope.pb.h"

struct MessageBuffer {
    uint8_t* ptr;
    uint32_t size;
};

class RpcServer {
    int server_fd, client_fd;

public:
    RpcServer(int port) {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) perror("socket");

        // Set socket option to reuse address
        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("bind");
            exit(EXIT_FAILURE);
        }

        if (listen(server_fd, 1) < 0) {
            perror("listen");
            exit(EXIT_FAILURE);
        }

        printf("[Server] Waiting for client...\n");
        client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) perror("accept");

        printf("[Server] Client connected\n");
    }


    ~RpcServer() {
        close(client_fd);
        close(server_fd);
    }

    void HandleIncoming() {
        uint32_t len;
        int r = recv(client_fd, &len, sizeof(len), MSG_WAITALL);
        if (r <= 0) {
            printf("No message length received\n");
            return;
        }

        uint8_t buf[256];
        r = recv(client_fd, buf, len, MSG_WAITALL);
        if (r <= 0) {
            printf("No message payload received\n");
            return;
        }

        RpcEnvelope envelope = RpcEnvelope_init_zero;
        pb_istream_t stream = pb_istream_from_buffer(buf, len);

        if (!pb_decode(&stream, RpcEnvelope_fields, &envelope)) {
            printf("Failed to decode RpcEnvelope: %s\n", PB_GET_ERROR(&stream));
            return;
        }

        const char* method = envelope.method;
        if (strcmp(method, "SendMessage") == 0) {
            handle_SendMessage(envelope.request_id, envelope.payload.bytes, envelope.payload.size);
        } else {
            printf("Unknown method: %s\n", method);
        }
    }

private:
    void handle_SendMessage(uint32_t request_id, const void* payload_data, size_t payload_size) {
        MyMessage msg = MyMessage_init_zero;
        pb_istream_t payload_stream = pb_istream_from_buffer((const pb_byte_t*)payload_data, payload_size);
        if (!pb_decode(&payload_stream, MyMessage_fields, &msg)) {
            printf("Failed to decode MyMessage: %s\n", PB_GET_ERROR(&payload_stream));
            return;
        }

        printf("[Server] Received msg: id=%d name=\"%s\"\n", msg.id, msg.name);

        Ack ack = Ack_init_zero;
        ack.success = true;
        snprintf(ack.info, sizeof(ack.info), "Got message from '%s'", msg.name);

        send_ack_response(request_id, ack);
    }

    void send_ack_response(uint32_t request_id, const Ack& ack) {
        uint8_t payload_buf[128];
        pb_ostream_t payload_stream = pb_ostream_from_buffer(payload_buf, sizeof(payload_buf));
        if (!pb_encode(&payload_stream, Ack_fields, &ack)) {
            printf("Failed to encode Ack: %s\n", PB_GET_ERROR(&payload_stream));
            return;
        }

        RpcResponse response = RpcResponse_init_zero;
        response.request_id = request_id;
        memcpy(response.payload.bytes, payload_buf, payload_stream.bytes_written);
        response.payload.size = payload_stream.bytes_written;
        strncpy(response.status, "OK", sizeof(response.status) - 1);

        uint8_t buf[256];
        pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(buf));
        if (!pb_encode(&stream, RpcResponse_fields, &response)) {
            printf("Failed to encode RpcResponse: %s\n", PB_GET_ERROR(&stream));
            return;
        }

        uint32_t len = stream.bytes_written;
        send(client_fd, &len, sizeof(len), 0);
        send(client_fd, buf, len, 0);
    }
};


int main() {
    printf("Server main function\n");

    RpcServer server(12345);
    server.HandleIncoming();

    return 0;
}

