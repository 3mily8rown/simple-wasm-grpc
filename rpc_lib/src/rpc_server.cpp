#include "rpc_server.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

void RpcServer::RegisterHandler(uint32_t tag, HandlerFn handler) {
    handlers_[tag] = std::move(handler);
}

RpcServer::Buffer RpcServer::alloc_buffer(uint32_t max_size) {
    uint8_t* ptr = static_cast<uint8_t*>(std::malloc(max_size));
    return { ptr, ptr ? max_size : 0 };
}

bool RpcServer::ProcessNextRequest() {
    // 1) Allocate buffer for incoming message
    Buffer buf = alloc_buffer(512);
    if (!buf.data) {
        std::fprintf(stderr, "[Server] malloc failed\n");
        return false;
    }

    // 2) Receive message from WASM
    int32_t len = receive_rpcmessage(reinterpret_cast<uint32_t>(buf.data), buf.size);
    if (len <= 0) {
        std::printf("[Server] No message received (timeout or error)\n");
        std::free(buf.data);
        return false;
    }

    // 3) Decode the RpcEnvelope
    RpcEnvelope env = RpcEnvelope_init_zero;
    pb_istream_t istream = pb_istream_from_buffer(buf.data, len);
    if (!pb_decode(&istream, RpcEnvelope_fields, &env)) {
        std::fprintf(stderr, "[Server] RpcEnvelope decode error: %s\n", PB_GET_ERROR(&istream));
        std::free(buf.data);
        return false;
    }

    // 4) Dispatch by payload tag
    auto it = handlers_.find(env.which_payload);
    if (it != handlers_.end()) {
        bool handled = it->second(env.request_id, env);
        std::free(buf.data);
        return handled;
    } else {
        std::fprintf(stderr, "[Server] No handler registered for payload tag %d\n", env.which_payload);
        std::free(buf.data);
        return false;
    }
}

void RpcServer::sendResponse(const RpcResponse& resp) {
    uint8_t tmp[512];
    pb_ostream_t ostream = pb_ostream_from_buffer(tmp, sizeof(tmp));
    if (!pb_encode(&ostream, RpcResponse_fields, &resp)) {
        std::fprintf(stderr, "[Server] Response encode error: %s\n", PB_GET_ERROR(&ostream));
        return;
    }

    uint32_t len = (uint32_t)ostream.bytes_written;
    uint8_t* wasm_buf = static_cast<uint8_t*>(std::malloc(len));
    if (!wasm_buf) {
        std::fprintf(stderr, "[Server] malloc failed\n");
        return;
    }

    std::memcpy(wasm_buf, tmp, len);
    send_rpcresponse(reinterpret_cast<uint32_t>(wasm_buf), len);
    std::free(wasm_buf);
}
