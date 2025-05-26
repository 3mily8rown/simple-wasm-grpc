#include <stdio.h>
#include <cstdint>
#include <cstdlib>

#include "pb_encode.h"
#include "pb_decode.h"
#include "message.pb.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "rpc_envelope.pb.h"

extern int32_t send_rpcmessage(uint32_t offset, uint32_t length);
extern int32_t receive_rpcresponse(uint32_t offset, uint32_t length);


class RpcClient {
public:
    void Send(const char* method, const void* message, const pb_msgdesc_t* fields) {
        // Encode the payload
        uint8_t payload_buf[128];
        pb_ostream_t payload_stream = pb_ostream_from_buffer(payload_buf, sizeof(payload_buf));
        if (!pb_encode(&payload_stream, fields, message)) {
            printf("Failed to encode payload: %s\n", PB_GET_ERROR(&payload_stream));
            return;
        }

        size_t payload_len = payload_stream.bytes_written;

        // Create RpcEnvelope
        RpcEnvelope env = RpcEnvelope_init_zero;
        strncpy(env.method, method, sizeof(env.method) - 1);
        memcpy(env.payload.bytes, payload_buf, payload_len);
        env.payload.size = payload_len;

        // Encode RpcEnvelope
        uint8_t envelope_buf[256];
        pb_ostream_t env_stream = pb_ostream_from_buffer(envelope_buf, sizeof(envelope_buf));
        if (!pb_encode(&env_stream, RpcEnvelope_fields, &env)) {
            printf("Failed to encode RpcEnvelope: %s\n", PB_GET_ERROR(&env_stream));
            return;
        }

        // Allocate message buffer in WASM heap and copy
        uint8_t* wasm_buf = (uint8_t*)malloc(env_stream.bytes_written);
        if (!wasm_buf) {
            printf("Failed to malloc\n");
            return;
        }

        memcpy(wasm_buf, envelope_buf, env_stream.bytes_written);
        send_rpcmessage((uint32_t)wasm_buf, env_stream.bytes_written);
    }

    Ack ReceiveAck() {
        // Allocate buffer
        uint8_t* buf = (uint8_t*)malloc(256);
        int32_t len = receive_rpcresponse((uint32_t)buf, 256);

        if (len <= 0) {
            printf("No response received\n");
            Ack empty = Ack_init_zero;
            return empty;
        }

        // Decode RpcResponse
        RpcResponse resp = RpcResponse_init_zero;
        pb_istream_t stream = pb_istream_from_buffer(buf, len);
        if (!pb_decode(&stream, RpcResponse_fields, &resp)) {
            printf("Failed to decode RpcResponse: %s\n", PB_GET_ERROR(&stream));
            Ack empty = Ack_init_zero;
            return empty;
        }

        printf("[Client] Received RpcResponse (id=%u, status=%s)\n", resp.request_id, resp.status);

        // Decode payload as Ack
        Ack ack = Ack_init_zero;
        pb_istream_t ack_stream = pb_istream_from_buffer(resp.payload.bytes, resp.payload.size);
        if (!pb_decode(&ack_stream, Ack_fields, &ack)) {
            printf("Failed to decode Ack: %s\n", PB_GET_ERROR(&ack_stream));
            return Ack_init_zero;
        }

        return ack;
    }

};


int main() {
    printf("Client main function\n");

    RpcClient client;
    MyMessage msg = MyMessage_init_default;
    msg.id = 42;
    strcpy(msg.name, "hello from client");

    client.Send("SendMessage", &msg, MyMessage_fields);
    Ack ack = client.ReceiveAck();
    printf("[Client] Ack: success=%d, info=\"%s\"\n", ack.success, ack.info);

    return 0;
}

