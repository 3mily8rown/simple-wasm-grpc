#include <iostream>
#include <cstdint>
#include <array>
#include <cmath>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <wasm_export.h>

// Struggles with non absolute path even though it is included in the CMakeLists?
#include "/home/eb/fyp/my_repos/simple-wasm-grpc/proto_messages/generated_full/message.pb.h"

// guards message queue for push/pop
static std::mutex g_msg_mutex;
// (only whilst both local) so one thread waits until another signals
static std::condition_variable g_msg_cv;
// queue for messages
static std::queue<std::vector<uint8_t>> g_msg_queue;

void send_mymessage(wasm_exec_env_t exec_env, uint32_t offset, uint32_t length) {
    wasm_module_inst_t inst = wasm_runtime_get_module_inst(exec_env);
    uint8_t* src = static_cast<uint8_t*>(wasm_runtime_addr_app_to_native(inst, offset));
    if (!src) {
        printf("Invalid memory address\n");
        return;
    }

    std::vector<uint8_t> data(src, src + length);

    // Put into send queue
    {
        std::lock_guard<std::mutex> lk(g_msg_mutex);
        g_msg_queue.emplace(std::move(data));
    }
    g_msg_cv.notify_one();
}

int32_t receive_mymessage(wasm_exec_env_t exec_env, uint32_t offset, uint32_t max_length) {
    wasm_module_inst_t inst = wasm_runtime_get_module_inst(exec_env);
    uint8_t* dest = static_cast<uint8_t*>(wasm_runtime_addr_app_to_native(inst, offset));
    if (!dest) {
        printf("Invalid memory address\n");
        return static_cast<int32_t>(0);
    }

    // wait until message in queue
    std::unique_lock<std::mutex> lk(g_msg_mutex);
    g_msg_cv.wait(lk, []{ return !g_msg_queue.empty(); });

    auto data = std::move(g_msg_queue.front());
    g_msg_queue.pop();
    lk.unlock();

    // copy up to max_length bytes
    uint32_t to_copy = std::min<uint32_t>(data.size(), max_length);
    memcpy(dest, data.data(), to_copy);

    // return actual message length in the buffer
    return static_cast<int32_t>(to_copy);
}
