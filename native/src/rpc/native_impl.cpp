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

#include "rpc/message_queue.h"
#include "rpc/socket_communication.h"

std::atomic<bool> g_local_consumer_online = false;
std::atomic<bool> g_local_client_online = false;

void send_rpcmessage(wasm_exec_env_t exec_env, uint32_t offset, uint32_t length) {
    wasm_module_inst_t inst = wasm_runtime_get_module_inst(exec_env);
    uint8_t* src = static_cast<uint8_t*>(wasm_runtime_addr_app_to_native(inst, offset));

    if (!src) {
        printf("Invalid memory address\n");
        return;
    }
    if (g_local_consumer_online.load(std::memory_order_acquire)) {
        printf("Local server is online, sending message...\n");
        queue_message(src, length);
    } else {
        printf("Local server is offline, sending over socket...\n");
        send_over_socket(src, length);
    }    
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

    return dequeue_message(dest, max_length);
}

void send_rpcresponse(wasm_exec_env_t exec_env, uint32_t offset, uint32_t length) {
    wasm_module_inst_t inst = wasm_runtime_get_module_inst(exec_env);
    uint8_t* src = static_cast<uint8_t*>(wasm_runtime_addr_app_to_native(inst, offset));

    if (!src) {
        printf("Invalid memory address\n");
        return;
    }
    if (g_local_client_online.load(std::memory_order_acquire)) {
        printf("Local client is online, sending message...\n");
        queue_response(src, length);
    } else {
        printf("Local client is offline, sending over socket...\n");
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
