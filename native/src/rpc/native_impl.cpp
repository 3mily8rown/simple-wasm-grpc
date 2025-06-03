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

#include "ring_buffer_rpc/rpc_messaging.h"
#include "rpc/socket_communication.h"
#include "rpc/native_impl.h"
#include "wamr/thread_launch.h"

std::atomic<bool> g_local_consumer_online = false;
std::atomic<bool> g_local_client_online = false;
bool colocated = false;

// todo for async do the clients/servers just need to know their id? orther wise hwo would a client calling receive ever be paired to their initial request
int32_t send_rpcmessage(wasm_exec_env_t exec_env, uint32_t offset, uint32_t length) {
    wasm_module_inst_t inst = wasm_runtime_get_module_inst(exec_env);
    uint8_t* src = static_cast<uint8_t*>(wasm_runtime_addr_app_to_native(inst, offset));

    if (!src) {
        printf("Invalid memory address\n");
        return -1;
    }
    if (length < 4) {
        printf("Buffer too small for request_id prefix\n");
        return -1;
    }

    uint32_t request_id = get_thread_id();
    begin_wait_for_response(request_id);
    std::cout << "[Native] Sending RPC message with request_id: " << request_id << "\n";

    // Add request_id to your message frame
    std::memcpy(src, &request_id, sizeof(uint32_t));

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

    return dequeue_message_with_timeout(dest, max_length, 5000);
}

int32_t send_rpcmessage_unary(wasm_exec_env_t exec_env,
                                  uint32_t offset,
                                  uint32_t length,
                                  uint32_t resp_offset,
                                  uint32_t max_len) {
    wasm_module_inst_t inst = wasm_runtime_get_module_inst(exec_env);
    uint8_t* req_buf = static_cast<uint8_t*>(wasm_runtime_addr_app_to_native(inst, offset));
    uint8_t* resp_buf = static_cast<uint8_t*>(wasm_runtime_addr_app_to_native(inst, resp_offset));

    if (!req_buf || !resp_buf) {
        printf("Invalid memory address\n");
        return 0;
    }

    uint32_t request_id = get_thread_id(); // Get the thread ID as request ID

    // Add request_id to your message frame
    // (e.g., encode it into a header, prefix, etc.)
    // For now you can inject it into the first 4 bytes:
    std::memcpy(req_buf, &request_id, sizeof(uint32_t));

    // Register response slot
    begin_wait_for_response(request_id);

    // Send
    queue_message(req_buf, length);

    // Wait for response
    std::vector<uint8_t> response;
    if (!wait_for_response(request_id, response, 10000)) {
        std::fprintf(stderr, "[Client] Timeout waiting for response\n");
        return 0;
    }

    uint32_t copy_len = std::min(max_len, static_cast<uint32_t>(response.size()));
    std::memcpy(resp_buf, response.data(), copy_len);
    return copy_len;
}

void send_rpcresponse(wasm_exec_env_t exec_env, uint32_t offset, uint32_t length, uint32_t request_id) {
    wasm_module_inst_t inst = wasm_runtime_get_module_inst(exec_env);
    uint8_t* src = static_cast<uint8_t*>(wasm_runtime_addr_app_to_native(inst, offset));

    if (!src) {
        printf("Invalid memory address\n");
        return;
    }
    if (g_local_client_online.load(std::memory_order_acquire)) {
        // printf("Local client is online, sending message...\n");
        deliver_response(request_id , src, length);
    } else if (colocated) {
        // printf("Colocated environment detected, client not ready...\n");
        deliver_response(request_id , src, length);
    } else {
        // printf("Local client is offline, sending over socket...\n");
        // todo incorporate request_id into socket communication
        send_response_over_socket(src, length, request_id);
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

    // todo If want non unary assumption - need to know a message id not just thread
    uint32_t request_id = get_thread_id();

    std::vector<uint8_t> response;
    if (!wait_for_response(request_id, response, /*timeout_ms=*/10000)) {
        std::fprintf(stderr, "[Client] Timed out waiting for response\n");
        return 0;
    }

    uint32_t copy_len = std::min(max_len, static_cast<uint32_t>(response.size()));
    std::memcpy(dest, response.data(), copy_len);
    return copy_len;
}

// ----------------------------------------------------------
// Metrics functions

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
