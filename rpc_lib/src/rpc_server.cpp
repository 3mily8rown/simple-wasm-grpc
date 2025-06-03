#include "rpc_server.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

// ---------------- Core Infrastructure ----------------

void RpcServer::RegisterHandler(uint32_t tag, HandlerFn handler) {
    handlers_[tag] = std::move(handler);
}

RpcServer::Buffer RpcServer::alloc_buffer(uint32_t max_size) {
    uint8_t* ptr = static_cast<uint8_t*>(std::malloc(max_size));
    return { ptr, ptr ? max_size : 0 };
}

bool RpcServer::ProcessNextRequest() {
    Buffer buf = alloc_buffer(512);
    if (!buf.data) {
        std::fprintf(stderr, "[Server] malloc failed\n");
        return false;
    }

    int32_t len = receive_rpcmessage(reinterpret_cast<uint32_t>(buf.data), buf.size);
    if (len <= 0) {
        std::printf("[Server] No message received (timeout or error)\n");
        std::free(buf.data);
        return false;
    }

    // Native set 4 bytes as request_id at the start of the buffer
    uint32_t request_id;
    std::memcpy(&request_id, buf.data, sizeof(uint32_t));

    // shift the payload to remove request_id if handlers shouldn't see it
    uint8_t* payload_start = buf.data + sizeof(uint32_t);
    int32_t payload_len = len - sizeof(uint32_t);

    RpcEnvelope env = RpcEnvelope_init_zero;
    pb_istream_t istream = pb_istream_from_buffer(payload_start, payload_len);
    if (!pb_decode(&istream, RpcEnvelope_fields, &env)) {
        std::fprintf(stderr, "[Server] RpcEnvelope decode error: %s\n", PB_GET_ERROR(&istream));
        std::free(buf.data);
        return false;
    }

    auto it = handlers_.find(env.which_payload);
    bool handled = false;
    if (it != handlers_.end()) {
        handled = it->second(request_id, env);
    } else {
        std::fprintf(stderr, "[Server] No handler registered for payload tag %d\n", env.which_payload);
    }

    std::free(buf.data);
    return handled;
}

void RpcServer::sendResponse(const RpcResponse& resp, uint32_t request_id) {
    uint8_t tmp[512];
    pb_ostream_t ostream = pb_ostream_from_buffer(tmp, sizeof(tmp));
    if (!pb_encode(&ostream, RpcResponse_fields, &resp)) {
        std::fprintf(stderr, "[Server] Response encode error: %s\n", PB_GET_ERROR(&ostream));
        return;
    }

    uint32_t len = ostream.bytes_written;
    uint8_t* wasm_buf = static_cast<uint8_t*>(std::malloc(len));
    if (!wasm_buf) {
        std::fprintf(stderr, "[Server] malloc failed\n");
        return;
    }

    std::memcpy(wasm_buf, tmp, len);
    send_rpcresponse(reinterpret_cast<uint32_t>(wasm_buf), len, request_id);
    std::free(wasm_buf);
}

// ---------------- Payload Extraction ----------------

template<>
const SendMessage& getPayload<SendMessage>(uint32_t, const RpcEnvelope& env) {
    return env.payload.msg;
}
template<>
const AddRandom& getPayload<AddRandom>(uint32_t, const RpcEnvelope& env) {
    return env.payload.rand;
}
template<>
const ProcessFloats& getPayload<ProcessFloats>(uint32_t, const RpcEnvelope& env) {
    return env.payload.flt;
}

// ---------------- Template Typed Handler ----------------

template<typename Req, typename Resp>
void RpcServer::registerTypedHandler(uint32_t tag, std::function<void(const Req&, Resp*)> handler) {
    RegisterHandler(tag, [=](uint32_t req_id, const RpcEnvelope& env) -> bool {
        const Req& req = getPayload<Req>(tag, env);
        Resp resp{};
        handler(req, &resp);

        RpcResponse out = RpcResponse_init_zero;
        out.request_id = req_id;
        out.status = true;
        out.which_payload = tag;

        if constexpr (std::is_same_v<Resp, SendMessageResponse>) {
            out.payload.msg = resp;
        } else if constexpr (std::is_same_v<Resp, AddRandomResponse>) {
            out.payload.rand = resp;
        } else if constexpr (std::is_same_v<Resp, ProcessFloatsResponse>) {
            out.payload.flt = resp;
        } else {
            static_assert(sizeof(Resp) == 0, "Unsupported response type in registerTypedHandler");
        }

        sendResponse(out, req_id);
        return true;
    });
}

// ---------------- Business Logic Registration ----------------

void RpcServer::registerFunction(uint32_t tag, std::function<std::string(int32_t, std::string)> fn) {
    registerTypedHandler<SendMessage, SendMessageResponse>(tag,
        [=](const SendMessage& msg, SendMessageResponse* resp) {
            std::string reply = fn(msg.id, msg.name);
            std::strncpy(resp->info, reply.c_str(), sizeof(resp->info) - 1);
        });
}

void RpcServer::registerFunction(uint32_t tag, std::function<int32_t(int32_t)> fn) {
    registerTypedHandler<AddRandom, AddRandomResponse>(tag,
        [=](const AddRandom& req, AddRandomResponse* resp) {
            resp->sum = fn(req.num);
        });
}

void RpcServer::registerFunction(uint32_t tag, std::function<float(std::vector<float>)> fn) {
    registerTypedHandler<ProcessFloats, ProcessFloatsResponse>(tag,
        [=](const ProcessFloats& req, ProcessFloatsResponse* resp) {
            std::vector<float> data(req.num, req.num + req.num_count);
            resp->sum = fn(std::move(data));
        });
}

// ---------------- Explicit Template Instantiations ----------------

template void RpcServer::registerTypedHandler<SendMessage, SendMessageResponse>(uint32_t, std::function<void(const SendMessage&, SendMessageResponse*)>);
template void RpcServer::registerTypedHandler<AddRandom, AddRandomResponse>(uint32_t, std::function<void(const AddRandom&, AddRandomResponse*)>);
template void RpcServer::registerTypedHandler<ProcessFloats, ProcessFloatsResponse>(uint32_t, std::function<void(const ProcessFloats&, ProcessFloatsResponse*)>);
