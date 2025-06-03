// ring_buffer_rpc/rpc_messaging.cpp

#include "ring_buffer_rpc/rpc_messaging.h"
#include "ring_buffer_rpc/ring_buffer.h"
#include <cstdint>
#include <cstring>
#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <unordered_map>

struct ResponseSlot {
    std::vector<uint8_t> response;
    std::mutex mtx;
    std::condition_variable cv;
    bool ready = false;

    // Waits for data or times out
    bool wait_for_data(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mtx);
        return cv.wait_for(lock, timeout, [&]() { return ready; });
    }
};

// ------------------------------------------------------------------------------------------------------------
// global ring buffer (requests) and map of response slots
// ------------------------------------------------------------------------------------------------------------
static constexpr size_t REQUEST_BUFFER_CAPACITY  = 1 * 1024 * 1024; // 1 MiB
static RingBuffer g_request_rb (REQUEST_BUFFER_CAPACITY);


static std::unordered_map<uint32_t, ResponseSlot*> g_response_slots;
static std::mutex g_response_slots_mtx;


// ------------------------------------------------------------------------------------------------------------
// Public API
// ------------------------------------------------------------------------------------------------------------

// Enqueue a request. If it doesn't fit, print an error.
void queue_message(const uint8_t* data, uint32_t length) {
    // Check “never‐fit” condition first
    if (uint64_t(length) + sizeof(uint32_t) > REQUEST_BUFFER_CAPACITY - 1) {
        std::cerr << "[RingBuffer] queue_message: message too large ("
                  << length << " bytes)\n";
        return;
    }
    bool ok = g_request_rb.enqueue(data, length);
    if (!ok) {
        std::cerr << "[RingBuffer] queue_message: not enough space for "
                  << length << "‐byte message\n";
    }
}

// Dequeue exactly one request. BLOCKS until a complete message is available.
// Returns the actual payload length (caller can compare against max_length
// to see if truncation occurred, but in normal operation payload ≤ max_length).
uint32_t dequeue_message(uint8_t* dest, uint32_t max_length) {
    return g_request_rb.dequeue(dest, max_length);
}

int32_t dequeue_message_with_timeout(uint8_t* dest, uint32_t max_length, int timeout_ms) {
    return g_request_rb.dequeue_with_timeout(dest, max_length, timeout_ms);
}

void begin_wait_for_response(uint32_t request_id) {
    auto* slot = new ResponseSlot();
    std::lock_guard<std::mutex> lock(g_response_slots_mtx);
    g_response_slots[request_id] = slot;
}


bool wait_for_response(uint32_t request_id, std::vector<uint8_t>& out, int timeout_ms) {
    ResponseSlot* slot;

    {
        std::lock_guard<std::mutex> lock(g_response_slots_mtx);
        auto it = g_response_slots.find(request_id);
        if (it == g_response_slots.end()) return false;
        slot = it->second;
    }

    bool ok = slot->wait_for_data(std::chrono::milliseconds(timeout_ms));

    {
        std::lock_guard<std::mutex> lock(g_response_slots_mtx);
        g_response_slots.erase(request_id);
    }

    if (!ok) return false;
    out = std::move(slot->response);
    delete slot; // clean up
    return true;
}


void deliver_response(uint32_t request_id, const uint8_t* data, uint32_t len) {
    std::unique_lock<std::mutex> lock(g_response_slots_mtx);
    auto it = g_response_slots.find(request_id);
    if (it == g_response_slots.end()) {
        std::cerr << "[RingBuffer] deliver_response: unknown request_id " << request_id << "\n";
        return;
    }


    ResponseSlot* slot = it->second;
    g_response_slots.erase(it); // one-shot
    lock.unlock();

    {
        std::lock_guard<std::mutex> slot_lock(slot->mtx);
        slot->response.assign(data, data + len);
        slot->ready = true;
    }
    slot->cv.notify_one();
}

