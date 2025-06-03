#ifndef RPC_CLIENT_H
#define RPC_CLIENT_H

#include <cstdint>
#include <vector>
#include <string>
#include "rpc_envelope.pb.h"

extern "C" {
    int32_t send_rpcmessage(uint32_t offset, uint32_t length);
    int32_t receive_rpcresponse(uint32_t offset, uint32_t length);
}

class RpcClient {
public:
    RpcClient() = default;

    // Public RPC interface
    std::string sendMessage(int32_t id, const char* name);           // Returns ack.info
    int32_t addRandom(int32_t num);                                  // Returns sum
    float processFloats(const float* arr, size_t count);             // Returns sum
    float processFloats(const std::vector<float>& vec);              // Convenience wrapper

private:
    // Internal helpers
    bool send(uint32_t request_id, uint32_t payload_tag, const void* message);
    
    template<typename T>
    bool receive(uint32_t expected_tag, T* out_msg);

    // Optional: ID counter if you want to auto-generate request IDs
    uint32_t next_request_id_ = 1;
};

#endif // RPC_CLIENT_H
