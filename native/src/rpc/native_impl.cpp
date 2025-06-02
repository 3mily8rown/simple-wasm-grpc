#include <iostream>
#include <cstdint>
#include <array>
#include <cmath>
#include <mutex>
#include <condition_variable>
#include <cstring>
#include <queue>
#include <vector>
#include <wasm_export.h>
#include <atomic>

// #include "rpc/message_queue.h"
#include "ring_buffer_rpc/rb_message_queue.h"
#include "rpc/socket_communication.h"
#include "rpc/native_impl.h"

std::atomic<bool> g_local_consumer_online = false;
std::atomic<bool> g_local_client_online = false;
bool colocated = false;

int32_t send_rpcmessage(wasm_exec_env_t exec_env, uint32_t offset, uint32_t length) {
    wasm_module_inst_t inst = wasm_runtime_get_module_inst(exec_env);
    uint8_t* src = static_cast<uint8_t*>(wasm_runtime_addr_app_to_native(inst, offset));

    if (!src) {
        printf("Invalid memory address\n");
        return -1;
    }
    if (g_local_consumer_online.load(std::memory_order_acquire)) {
        // printf("Local server is online, sending message...\n");
        queue_message(src, length);
    } else if (colocated) {
        // printf("Colocated environment detected, server not ready...\n");
        queue_message(src, length);
    } else {
        // printf("Local server is offline, sending over socket...\n");
        send_over_socket(src, length);
    }    
    return 0;
}

int32_t receive_rpcmessage(wasm_exec_env_t exec_env, uint32_t offset, uint32_t max_length) {
    extern std::atomic<bool> g_local_consumer_online;
    g_local_consumer_online.store(true, std::memory_order_release);

    wasm_module_inst_t inst = wasm_runtime_get_module_inst(exec_env);
    uint8_t* dest = static_cast<uint8_t*>(wasm_runtime_addr_app_to_native(inst, offset));
    if (!dest) {
        printf("Invalid memory address\n");
        return 0;
    }

    return dequeue_message_with_timeout(dest, max_length, 3000);
}

void send_rpcresponse(wasm_exec_env_t exec_env, uint32_t offset, uint32_t length) {
    wasm_module_inst_t inst = wasm_runtime_get_module_inst(exec_env);
    uint8_t* src = static_cast<uint8_t*>(wasm_runtime_addr_app_to_native(inst, offset));

    if (!src) {
        printf("Invalid memory address\n");
        return;
    }
    if (g_local_client_online.load(std::memory_order_acquire)) {
        // printf("Local client is online, sending message...\n");
        queue_response(src, length);
    } else if (colocated) {
        // printf("Colocated environment detected, client not ready...\n");
        queue_response(src, length);
    } else {
        // printf("Local client is offline, sending over socket...\n");
        send_response_over_socket(src, length);
    }    
}

int32_t receive_rpcresponse(wasm_exec_env_t exec_env, uint32_t offset, uint32_t max_len) {
    extern std::atomic<bool> g_local_client_online;
    g_local_client_online.store(true, std::memory_order_release);

    wasm_module_inst_t inst = wasm_runtime_get_module_inst(exec_env);
    uint8_t* dest = static_cast<uint8_t*>(wasm_runtime_addr_app_to_native(inst, offset));
    if (!dest) {
        printf("Invalid memory address\n");
        return 0;
    }

    return dequeue_response(dest, max_len);
}

int64_t get_time_us(wasm_exec_env_t exec_env) {
    using namespace std::chrono;
    auto now = high_resolution_clock::now();
    return duration_cast<microseconds>(now.time_since_epoch()).count();
}

void send_rtt(wasm_exec_env_t exec_env, uint32_t time_us) {
    std::cout << "[METRICS] RTT = " << time_us << "μs" << std::endl;
}

void send_total(wasm_exec_env_t exec_env, uint32_t time_us, uint32_t count) {
    uint32_t throughput = (count > 0) ? (time_us / count) : 0;
    std::cout << "[METRICS] THROUGHPUT = " << throughput << "μs" << std::endl;
}
