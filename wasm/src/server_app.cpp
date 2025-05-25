#include <stdio.h>
#include <cstdint>
#include <cstdlib>

#include "pb_encode.h"
#include "pb_decode.h"
#include "message.pb.h"

#include "rpc_envelope.pb.h"
#include <string.h>

extern int32_t receive_rpcmessage(uint32_t offset, uint32_t length);

struct MessageBuffer {
    uint8_t* ptr;
    uint32_t size;
};

class RpcServer {
public:
    void HandleIncoming() {
        // Allocate buffer in WASM
        MessageBuffer buf = receive_buffer(256); // adjustable max size
        int32_t actual_size = receive_rpcmessage((uint32_t)buf.ptr, buf.size);
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
            handle_SendMessage(envelope.payload.bytes, envelope.payload.size);
        } else {        // Handle other methods or unknown method
            printf("Unknown method: %s\n", method);
        }

        free(buf.ptr);
    }

private:
    MessageBuffer receive_buffer(uint32_t max_length) {
        uint8_t* buffer_ptr = (uint8_t*)malloc(max_length);
        if (!buffer_ptr) return {nullptr, 0};
        return {buffer_ptr, max_length};
    }

    void handle_SendMessage(const void* payload_data, size_t payload_size) {
        MyMessage msg = MyMessage_init_zero;
        pb_istream_t payload_stream = pb_istream_from_buffer((const pb_byte_t*)payload_data, payload_size);
        if (!pb_decode(&payload_stream, MyMessage_fields, &msg)) {
            printf("Failed to decode MyMessage: %s\n", PB_GET_ERROR(&payload_stream));
            return;
        }

        printf("[Server] Received msg: id=%d name=\"%s\"\n", msg.id, msg.name);

        // TODO: encode response and return via host
    }
};


int main() {
    printf("Server main function\n");

    RpcServer server;
    server.HandleIncoming();

    return 0;
}
