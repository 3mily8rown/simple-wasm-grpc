// ring_buffer_rpc/rb_message_queue.h
#ifndef RPC_MESSAGE_QUEUE_H
#define RPC_MESSAGE_QUEUE_H

#include <cstdint>

// Same API as before:
void queue_message(const uint8_t* data, uint32_t length);
uint32_t dequeue_message(uint8_t* dest, uint32_t max_length);

void queue_response(const uint8_t* data, uint32_t length);
uint32_t dequeue_response(uint8_t* dest, uint32_t max_length);

#endif // RPC_MESSAGE_QUEUE_H
