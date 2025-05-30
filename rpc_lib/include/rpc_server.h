#ifndef RPC_SERVER_H
#define RPC_SERVER_H

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include "pb_decode.h"
#include "pb_encode.h"
#include "rpc_envelope.pb.h"

// These are provided by your WASM host:
extern "C" {
    int32_t receive_rpcmessage(uint32_t offset, uint32_t length);
    void    send_rpcresponse(uint32_t offset, uint32_t length);
}

class RpcServer {
public:
    using HandlerFn = std::function<bool(uint32_t /*request_id*/,
                                         const void* /*payload_bytes*/,
                                         size_t /*payload_size*/)>;

    RpcServer() = default;
    ~RpcServer() = default;

    // Register a handler for a given RPC method name.
    // The handler should decode payload itself and then send a response.
    void RegisterHandler(const std::string& method, HandlerFn handler);

    // Blocks waiting for one incoming envelope, dispatches to the right handler.
    void ProcessNextRequest();

    void sendResponse(const RpcResponse& resp);

private:
    std::unordered_map<std::string, HandlerFn> handlers_;
    
    // Internal helpers to read raw bytes into wasm heap and decode the envelope.
    struct Buffer { uint8_t* data; uint32_t size; };
    Buffer alloc_buffer(uint32_t max_size);
};

#endif // RPC_SERVER_H
