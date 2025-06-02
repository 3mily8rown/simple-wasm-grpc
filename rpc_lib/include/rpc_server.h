#ifndef RPC_SERVER_H
#define RPC_SERVER_H

#include <cstdint>
#include <functional>
#include <unordered_map>
#include "pb_decode.h"
#include "pb_encode.h"
#include "rpc_envelope.pb.h"

extern "C" {
    int32_t receive_rpcmessage(uint32_t offset, uint32_t length);
    void    send_rpcresponse(uint32_t offset, uint32_t length);
}

class RpcServer {
public:
    // New: Dispatch handlers based on which_payload tag
    using HandlerFn = std::function<bool(uint32_t /*request_id*/, const RpcEnvelope&)>;

    RpcServer() = default;
    ~RpcServer() = default;

    void RegisterHandler(uint32_t payload_tag, HandlerFn handler);

    // Receives a request and dispatches based on RpcEnvelope.which_payload
    bool ProcessNextRequest();

    void sendResponse(const RpcResponse& resp);

private:
    std::unordered_map<uint32_t, HandlerFn> handlers_;

    // Allocates a receive buffer in WASM memory
    struct Buffer { uint8_t* data; uint32_t size; };
    Buffer alloc_buffer(uint32_t max_size);
};

#endif // RPC_SERVER_H
