#ifndef RPC_MESSAGING_H
#define RPC_MESSAGING_H

#include <cstdint>
#include <vector>

// Request API
void queue_message(const uint8_t* data, uint32_t length);
uint32_t dequeue_message(uint8_t* dest, uint32_t max_length);
int32_t dequeue_message_with_timeout(uint8_t* dest, uint32_t max_length, int timeout_ms);

// Response API (clean and safe)
void begin_wait_for_response(uint32_t request_id);  // ✅ Add this
bool wait_for_response(uint32_t request_id, std::vector<uint8_t>& out, int timeout_ms);
void deliver_response(uint32_t request_id, const uint8_t* data, uint32_t len);

#endif // RPC_MESSAGE_QUEUE_H
