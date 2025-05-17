#include <stdio.h>
#include <cstdint>
#include <cstdlib>

#include "pb_encode.h"
#include "pb_decode.h"
#include "message.pb.h"

int32_t receive_rpcmessage(uint32_t offset, uint32_t length);

struct MessageBuffer {
    uint8_t* ptr;
    uint32_t size;
};

MessageBuffer receive_message_buffer(uint32_t max_length) {
    // Allocate memory inside the WASM module's heap
    uint32_t size = max_length;
    uint8_t* buffer_ptr = (uint8_t*)malloc(size);
    if (!buffer_ptr) return {nullptr, 0};
    return {buffer_ptr, size};
}

void receive_message(uint32_t max_length) {
    // Make buffer where want to receive message
    MessageBuffer buf = receive_message_buffer(max_length);

    int32_t actual_size = receive_rpcmessage((uint32_t)buf.ptr, buf.size);

    // Turn linear memory into nanopb stream
    pb_istream_t stream = pb_istream_from_buffer((pb_byte_t*)(uintptr_t)buf.ptr, actual_size);

    // TODO atm this specifically expects MyMessage - can we change that
    // Decode into nanopb struct:
    MyMessage msg = MyMessage_init_zero;
    if (!pb_decode(&stream, MyMessage_fields, &msg)) {
        printf("Wasm decode failed: %s\n", PB_GET_ERROR(&stream));
        return;
    }
    
    printf("Server recv: \"%s\"\n", msg.name);
    fflush(stdout);

    // TODO send Ack?
}

void receive_message() {
    receive_message((uint32_t)128);
}

int main() {
    printf("Server main function\n");
    fflush(stdout);
    receive_message();
    return 0;
}