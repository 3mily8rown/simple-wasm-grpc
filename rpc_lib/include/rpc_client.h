#ifndef RPC_CLIENT_H
#define RPC_CLIENT_H

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include "pb_encode.h"
#include "pb_decode.h"
#include "rpc_envelope.pb.h"
#include "message.pb.h"

extern "C" {
    int32_t send_rpcmessage(uint32_t offset, uint32_t length);
    int32_t receive_rpcresponse(uint32_t offset, uint32_t length);
}

class RpcClient {
public:
    RpcClient() = default;
    ~RpcClient() = default;

    bool Send(uint32_t request_id,
              uint32_t payload_tag,
              const void* message);

    template<typename T>
    bool Receive(uint32_t expected_tag, T* out_msg);  // Declaration only
};

// ——— Specializations ———

template<>
inline bool RpcClient::Receive<SendMessageResponse>(uint32_t expected_tag, SendMessageResponse* out_msg) {
    uint8_t* buf = static_cast<uint8_t*>(std::malloc(512));
    if (!buf) return false;

    int32_t len = receive_rpcresponse(reinterpret_cast<uint32_t>(buf), 512);
    if (len <= 0) {
        std::free(buf);
        return false;
    }

    RpcResponse resp = RpcResponse_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(buf, len);
    if (!pb_decode(&stream, RpcResponse_fields, &resp)) {
        std::fprintf(stderr, "Response decode error: %s\n", PB_GET_ERROR(&stream));
        std::free(buf);
        return false;
    }

    if (resp.which_payload != expected_tag) {
        std::fprintf(stderr, "Unexpected response type: got %d, expected %d\n",
                     resp.which_payload, expected_tag);
        std::free(buf);
        return false;
    }

    *out_msg = resp.payload.msg;
    std::free(buf);
    return true;
}

template<>
inline bool RpcClient::Receive<AddRandomResponse>(uint32_t expected_tag, AddRandomResponse* out_msg) {
    uint8_t* buf = static_cast<uint8_t*>(std::malloc(512));
    if (!buf) return false;

    int32_t len = receive_rpcresponse(reinterpret_cast<uint32_t>(buf), 512);
    if (len <= 0) {
        std::free(buf);
        return false;
    }

    RpcResponse resp = RpcResponse_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(buf, len);
    if (!pb_decode(&stream, RpcResponse_fields, &resp)) {
        std::fprintf(stderr, "Response decode error: %s\n", PB_GET_ERROR(&stream));
        std::free(buf);
        return false;
    }

    if (resp.which_payload != expected_tag) {
        std::fprintf(stderr, "Unexpected response type: got %d, expected %d\n",
                     resp.which_payload, expected_tag);
        std::free(buf);
        return false;
    }

    *out_msg = resp.payload.rand;
    std::free(buf);
    return true;
}

template<>
inline bool RpcClient::Receive<ProcessFloatsResponse>(uint32_t expected_tag, ProcessFloatsResponse* out_msg) {
    uint8_t* buf = static_cast<uint8_t*>(std::malloc(512));
    if (!buf) return false;

    int32_t len = receive_rpcresponse(reinterpret_cast<uint32_t>(buf), 512);
    if (len <= 0) {
        std::free(buf);
        return false;
    }

    RpcResponse resp = RpcResponse_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(buf, len);
    if (!pb_decode(&stream, RpcResponse_fields, &resp)) {
        std::fprintf(stderr, "Response decode error: %s\n", PB_GET_ERROR(&stream));
        std::free(buf);
        return false;
    }

    if (resp.which_payload != expected_tag) {
        std::fprintf(stderr, "Unexpected response type: got %d, expected %d\n",
                     resp.which_payload, expected_tag);
        std::free(buf);
        return false;
    }

    *out_msg = resp.payload.flt;
    std::free(buf);
    return true;
}

#endif // RPC_CLIENT_H
