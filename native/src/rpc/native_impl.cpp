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
#include "rpc/request_id_utils.h"

std::atomic<bool> g_local_consumer_online = false;
std::atomic<bool> g_local_client_online = false;
bool colocated = false;

// Function prototypes for internal use
int32_t send_rpcmessage_internal(uint8_t* src, uint32_t length, uint32_t request_id);
int32_t receive_rpcresponse_internal(uint8_t* dest, uint32_t max_len, uint32_t request_id);

void dump_response_slots(const char* context) {
    std::lock_guard<std::mutex> lock(g_response_slots_mtx);
    std::cerr << "[dump_response_slots] " << context << ": ";
    if (g_response_slots.empty()) {
        std::cerr << "No active slots.\n";
        return;
    }
    for (const auto& pair : g_response_slots) {
        std::cerr << "request_id=" << pair.first << " (slot ptr=" << pair.second << "), ";
    }
    std::cerr << "\n";
}

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
    dump_response_slots("[send_rpcmessage] Before sending");
    std::cout << "[Native] Sending RPC message with request_id: " << request_id << "\n";

    return send_rpcmessage_internal(src, length, request_id);
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

void send_rpcresponse(wasm_exec_env_t exec_env, uint32_t offset, uint32_t length, uint32_t request_id) {
    std::cout << "[Native] Sending RPC response for request_id: " << request_id << "\n";
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

    uint32_t request_id = get_thread_id();

    return receive_rpcresponse_internal(dest, max_len, request_id);
}

// Asynchronous RPC functions - to handle multiple outstanding requests per client

int32_t send_rpcmessage_with_id(wasm_exec_env_t exec_env, uint32_t offset,
    uint32_t length, uint32_t local_id) {
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

        uint32_t full_id = make_full_id(local_id, get_thread_id());
        begin_wait_for_response(full_id);
        dump_response_slots("[send_rpcmessage_with_id] Before sending");
        std::cout << "[Native] Sending RPC message with request_id: " << full_id << "\n";

        return send_rpcmessage_internal(src, length, full_id);
}

int32_t receive_rpcresponse_with_id(wasm_exec_env_t exec_env, uint32_t offset, 
    uint32_t max_len, uint32_t local_id) {
        extern std::atomic<bool> g_local_client_online;
        g_local_client_online.store(true, std::memory_order_release);

        wasm_module_inst_t inst = wasm_runtime_get_module_inst(exec_env);
        uint8_t* dest = static_cast<uint8_t*>(wasm_runtime_addr_app_to_native(inst, offset));
        if (!dest) {
            printf("Invalid memory address\n");
            return 0;
        }

        return receive_rpcresponse_internal(dest, max_len, make_full_id(local_id, get_thread_id()));
}

// private helper functions

int32_t send_rpcmessage_internal(uint8_t* src, uint32_t length, uint32_t request_id) {
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

int32_t receive_rpcresponse_internal(uint8_t* dest, uint32_t max_len, uint32_t request_id) {
    std::cout << "[Native] Waiting for response with request_id: " << request_id << "\n";
    std::vector<uint8_t> response;

    if (!wait_for_response(request_id, response, 10000)) {
        std::fprintf(stderr, "[Client] Timed out waiting for response\n");
        return 0;
    }

    std::cout << "[Native] Received response for request_id: " << request_id << ", size: " << response.size() << "\n";

    {
        std::cout << "[Native] Cleaning up response slot for request_id: " << request_id << "\n";
        dump_response_slots("[receive_rpcresponse_internal] Before cleanup");
        std::lock_guard<std::mutex> lock(g_response_slots_mtx);
        auto it = g_response_slots.find(request_id);
        if (it != g_response_slots.end()) {
            delete it->second;
            g_response_slots.erase(it);
            std::cout << "[receive_rpcresponse_internal] Cleaned up request_id: " << request_id << "\n";
        } else {
            std::cerr << "[receive_rpcresponse_internal] No slot found for request_id: " << request_id << "\n";
        }
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
