#include <mutex>
#include <condition_variable>
#include <queue>
#include <algorithm>
#include <cstring>

#include "rpc/message_queue.h"
#include <iostream>

static std::mutex g_msg_mutex;
static std::condition_variable g_msg_cv;
static std::queue<std::vector<uint8_t>> g_msg_queue;

void queue_message(const uint8_t* data, uint32_t length) {
    std::vector<uint8_t> message(data, data + length);
    {
        std::lock_guard<std::mutex> lock(g_msg_mutex);
        g_msg_queue.emplace(std::move(message));
    }
    g_msg_cv.notify_one();
}

// This function will block until a message is available
uint32_t dequeue_message(uint8_t* dest, uint32_t max_length) {
    std::unique_lock<std::mutex> lock(g_msg_mutex);

    if (g_msg_queue.empty()) {
        std::cout << "[Server] Message queue is empty\n";
    }

    g_msg_cv.wait(lock, [] { return !g_msg_queue.empty(); });

    auto message = std::move(g_msg_queue.front());
    g_msg_queue.pop();
    lock.unlock();

    uint32_t to_copy = std::min<uint32_t>(message.size(), max_length);
    std::memcpy(dest, message.data(), to_copy);
    return to_copy;
}
