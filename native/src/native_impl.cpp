#include <iostream>
#include <cstdint>
#include <array>
#include <cmath>
#include <wasm_export.h>

#include "/home/eb/fyp/my_repos/simple-wasm-grpc/proto_messages/generated_full/message.pb.h"

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

// TODO make it so the sent is queued then sent to whoever sent this
// for now this just makes a message and sends it
int32_t receive_rpcmessage(wasm_exec_env_t exec_env, uint32_t offset, uint32_t max_length) {
    // example message
    MyMessage msg;
    msg.set_id(42);
    msg.set_name("hello from native");

    std::string wire;
    if (!msg.SerializeToString(&wire)) {
        fprintf(stderr, "Protobuf serialize failed\n");
        return 0;
    }

    wasm_module_inst_t inst = wasm_runtime_get_module_inst(exec_env);
    uint8_t* memory = static_cast<uint8_t*>(wasm_runtime_addr_app_to_native(inst, offset));
    memcpy(memory, wire.data(), wire.size());
    return static_cast<uint32_t>(wire.size());
}

