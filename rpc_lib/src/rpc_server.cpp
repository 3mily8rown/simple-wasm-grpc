#include "rpc_server.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

void RpcServer::RegisterHandler(const std::string& method, HandlerFn handler) {
    handlers_[method] = std::move(handler);
}

RpcServer::Buffer RpcServer::alloc_buffer(uint32_t max_size) {
    uint8_t* ptr = (uint8_t*)std::malloc(max_size);
    return { ptr, ptr ? max_size : 0 };
}

void RpcServer::ProcessNextRequest() {
    // 1) Grab a buffer from the WASM heap
    Buffer buf = alloc_buffer(512);
    if (!buf.data) {
        std::fprintf(stderr, "[Server] malloc failed\n");
        return;
    }

    // 2) Receive the raw bytes
    int32_t len = receive_rpcmessage((uint32_t)buf.data, buf.size);
    if (len <= 0) {
        std::free(buf.data);
        return;
    }

    // 3) Decode the outer envelope
    RpcEnvelope env = RpcEnvelope_init_zero;
    pb_istream_t istream = pb_istream_from_buffer(buf.data, len);
    if (!pb_decode(&istream, RpcEnvelope_fields, &env)) {
        std::fprintf(stderr, "[Server] Envelope decode error: %s\n", PB_GET_ERROR(&istream));
        std::free(buf.data);
        return;
    }

    // 4) Dispatch to the registered handler
    std::string method(env.method);
    auto it = handlers_.find(method);
    bool handled = false;
    if (it != handlers_.end()) {
        handled = it->second(env.request_id, env.payload.bytes, env.payload.size);
    } else {
        std::fprintf(stderr, "[Server] No handler for method '%s'\n", env.method);
    }

    // 5) Free the receive buffer
    std::free(buf.data);
}

void RpcServer::sendResponse(const RpcResponse& resp) {
    // Encode into a temporary buffer
    uint8_t tmp[512];
    pb_ostream_t ostream = pb_ostream_from_buffer(tmp, sizeof(tmp));
    if (!pb_encode(&ostream, RpcResponse_fields, &resp)) {
        std::fprintf(stderr, "[Server] Response encode error: %s\n", PB_GET_ERROR(&ostream));
        return;
    }

    // Copy into WASM heap and send
    uint32_t len = (uint32_t)ostream.bytes_written;
    uint8_t* wasm_buf = (uint8_t*)std::malloc(len);
    if (!wasm_buf) {
        std::fprintf(stderr, "[Server] malloc failed\n");
        return;
    }
    std::memcpy(wasm_buf, tmp, len);
    send_rpcresponse((uint32_t)wasm_buf, len);
    std::free(wasm_buf);
}
