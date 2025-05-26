#include <stdio.h>
#include <cstdint>
#include <cstdlib>

#include "pb_encode.h"
#include "pb_decode.h"
#include "message.pb.h"

#include "rpc_envelope.pb.h"
#include <string.h>

extern int32_t receive_rpcmessage(uint32_t offset, uint32_t length);
extern void send_rpcresponse(uint32_t offset, uint32_t length);


struct MessageBuffer {
    uint8_t* ptr;
    uint32_t size;
};

class RpcServer {
public:
    void HandleIncoming() {
        // Allocate buffer in WASM
        MessageBuffer buf = receive_buffer(256); // adjustable max size
        printf("[Server] Waiting for message...\n");
        int32_t actual_size = receive_rpcmessage((uint32_t)buf.ptr, buf.size);
        printf("[Server] Received %d bytes\n", actual_size);
        if (actual_size <= 0) {
            printf("No message received\n");
            return;
        }

        // Decode envelope
        RpcEnvelope envelope = RpcEnvelope_init_zero;
        pb_istream_t stream = pb_istream_from_buffer(buf.ptr, actual_size);

        if (!pb_decode(&stream, RpcEnvelope_fields, &envelope)) {
            printf("Failed to decode RpcEnvelope: %s\n", PB_GET_ERROR(&stream));
            return;
        }

        // Now just access the fixed array directly (no .arg!)
        const char* method = envelope.method;

        if (strcmp(method, "SendMessage") == 0) {
            handle_SendMessage(envelope.request_id, envelope.payload.bytes, envelope.payload.size);
        } else {        // Handle other methods or unknown method
            printf("Unknown method: %s\n", method);
        }

        free(buf.ptr);
    }

    void send_ack_response(uint32_t request_id, const Ack& ack) {
        // Encode Ack payload
        uint8_t payload_buf[128];
        pb_ostream_t payload_stream = pb_ostream_from_buffer(payload_buf, sizeof(payload_buf));
        if (!pb_encode(&payload_stream, Ack_fields, &ack)) {
            printf("Failed to encode Ack: %s\n", PB_GET_ERROR(&payload_stream));
            return;
        }

        size_t payload_size = payload_stream.bytes_written;

        // Wrap in RpcResponse
        RpcResponse response = RpcResponse_init_zero;
        response.request_id = request_id;
        response.payload.size = payload_size;
        memcpy(response.payload.bytes, payload_buf, payload_size);
        strncpy(response.status, "OK", sizeof(response.status) - 1);

        // Encode RpcResponse
        uint8_t buf[256];
        pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(buf));
        if (!pb_encode(&stream, RpcResponse_fields, &response)) {
            printf("Failed to encode RpcResponse: %s\n", PB_GET_ERROR(&stream));
            return;
        }

        // Send buffer to host
        uint8_t* wasm_buf = (uint8_t*)malloc(stream.bytes_written);
        if (!wasm_buf) {
            printf("Failed to malloc response buffer\n");
            return;
        }

        memcpy(wasm_buf, buf, stream.bytes_written);
        send_rpcresponse((uint32_t)wasm_buf, stream.bytes_written);
    }


private:
    MessageBuffer receive_buffer(uint32_t max_length) {
        uint8_t* buffer_ptr = (uint8_t*)malloc(max_length);
        if (!buffer_ptr) return {nullptr, 0};
        return {buffer_ptr, max_length};
    }

    void handle_SendMessage(uint32_t request_id, const void* payload_data, size_t payload_size) {
        MyMessage msg = MyMessage_init_zero;
        pb_istream_t payload_stream = pb_istream_from_buffer((const pb_byte_t*)payload_data, payload_size);
        if (!pb_decode(&payload_stream, MyMessage_fields, &msg)) {
            printf("Failed to decode MyMessage: %s\n", PB_GET_ERROR(&payload_stream));
            return;
        }

        printf("[Server] Received msg: id=%d name=\"%s\"\n", msg.id, msg.name);

        // TODO: encode response and return via host
        // Construct Ack
        Ack ack = Ack_init_zero;
        ack.success = true;
        snprintf(ack.info, sizeof(ack.info), "Got message from '%s'", msg.name);

        send_ack_response(request_id, ack);
    }
};


int main() {
    printf("Server main function\n");

    RpcServer server;
    server.HandleIncoming();

    return 0;
}
