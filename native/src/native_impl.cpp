#include <iostream>
#include <cstdint>
#include <array>
#include <cmath>
#include <wasm_export.h>

#include "/home/eb/fyp/my_repos/simple-wasm-grpc/proto_messages/generated_full/message.pb.h"


void pass_to_native(wasm_exec_env_t exec_env, uint32_t offset, uint32_t length) {
    wasm_module_inst_t inst = wasm_runtime_get_module_inst(exec_env);
    uint8_t* memory = static_cast<uint8_t*>(wasm_runtime_addr_app_to_native(inst, offset));

    if (!memory) {
        printf("Invalid memory address\n");
        return;
    }

    std::string data((char*)memory, length);

    MyMessage msg;
    // MyMessage msg;
    if (!msg.ParseFromString(data)) {
        printf("Failed to parse protobuf\n");
        return;
    }

    std::cout << "Received message! ID = " << msg.id() << ", Name = " << msg.name() << std::endl;
}

void send_rpcmessage(wasm_exec_env_t exec_env, uint32_t offset, uint32_t length) {
    wasm_module_inst_t inst = wasm_runtime_get_module_inst(exec_env);
    uint8_t* memory = static_cast<uint8_t*>(wasm_runtime_addr_app_to_native(inst, offset));

    if (!memory) {
        printf("Invalid memory address\n");
        return;
    }

    std::string data((char*)memory, length);

    MyMessage msg;
    // MyMessage msg;
    if (!msg.ParseFromString(data)) {
        printf("Failed to parse protobuf\n");
        return;
    }

    std::cout << "Received message! ID = " << msg.id() << ", Name = " << msg.name() << std::endl;
}

