#include "rpc_client.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "pb.h"
#include "pb_decode.h"  // Required for pb_istream_from_buffer
#include "rpc_envelope.pb.h"  // Includes ProcessFloats struct
#include "pb_encode.h"

#define PB_ARRAY_SIZE(s, field) (sizeof((s)->field) / sizeof((s)->field[0]))

// ----------------------------
// Public API methods
// I think typical RPC libraries would generate these methods
// based on the protobuf definitions.
// The methods here are manually implemented for clarity and simplicity.
// ----------------------------

std::string RpcClient::sendMessage(int32_t id, const char* name) {
    SendMessage msg = SendMessage_init_zero;
    msg.id = id;
    std::strncpy(msg.name, name, sizeof(msg.name) - 1);

    if (!send(next_request_id_++, RpcEnvelope_msg_tag, &msg)) return {};

    SendMessageResponse resp = SendMessageResponse_init_zero;
    if (!receive<SendMessageResponse>(RpcResponse_msg_tag, &resp)) return {};

    return std::string(resp.info);
}

int32_t RpcClient::addRandom(int32_t num) {
    AddRandom msg = AddRandom_init_zero;
    msg.num = num;

    if (!send(next_request_id_++, RpcEnvelope_rand_tag, &msg)) return -1;

    AddRandomResponse resp = AddRandomResponse_init_zero;
    if (!receive<AddRandomResponse>(RpcResponse_rand_tag, &resp)) return -1;

    return resp.sum;
}

float RpcClient::processFloats(const float* arr, size_t count) {
    ProcessFloats msg = ProcessFloats_init_zero;

    if (count > PB_ARRAY_SIZE(&msg, num)) {
        std::fprintf(stderr, "Too many floats (max=%zu)\n", PB_ARRAY_SIZE(&msg, num));
        return -1.0f;
    }

    msg.num_count = count;
    std::memcpy(msg.num, arr, count * sizeof(float));

    if (!send(next_request_id_++, RpcEnvelope_flt_tag, &msg)) return -1.0f;

    ProcessFloatsResponse resp = ProcessFloatsResponse_init_zero;
    if (!receive<ProcessFloatsResponse>(RpcResponse_flt_tag, &resp)) return -1.0f;

    return resp.sum;
}


float RpcClient::processFloats(const std::vector<float>& vec) {
    return processFloats(vec.data(), vec.size());
}

// ----------------------------
// Internal helpers
// ----------------------------

bool RpcClient::send(uint32_t request_id, uint32_t payload_tag, const void* message) {
    RpcEnvelope env = RpcEnvelope_init_zero;
    env.request_id = request_id;
    env.which_payload = payload_tag;

    switch (payload_tag) {
        case RpcEnvelope_msg_tag:
            env.payload.msg = *reinterpret_cast<const SendMessage*>(message);
            break;
        case RpcEnvelope_rand_tag:
            env.payload.rand = *reinterpret_cast<const AddRandom*>(message);
            break;
        case RpcEnvelope_flt_tag:
            env.payload.flt = *reinterpret_cast<const ProcessFloats*>(message);
            break;
        default:
            std::fprintf(stderr, "Unknown payload_tag: %u\n", payload_tag);
            return false;
    }

    uint8_t buf[1024];
    pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(buf));
    if (!pb_encode(&stream, RpcEnvelope_fields, &env)) {
        std::fprintf(stderr, "RpcEnvelope encode error: %s\n", PB_GET_ERROR(&stream));
        return false;
    }

    uint32_t len = stream.bytes_written;
    uint8_t* wasm_buf = static_cast<uint8_t*>(std::malloc(len));
    if (!wasm_buf) {
        std::fprintf(stderr, "malloc failed\n");
        return false;
    }

    std::memcpy(wasm_buf, buf, len);
    int32_t rc = send_rpcmessage(reinterpret_cast<uint32_t>(wasm_buf), len);
    std::free(wasm_buf);

    return rc >= 0;
}

template<typename T>
bool RpcClient::receive(uint32_t expected_tag, T* out_msg) {
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

    if constexpr (std::is_same_v<T, SendMessageResponse>) {
        if (resp.which_payload != RpcResponse_msg_tag) return false;
        *out_msg = resp.payload.msg;
    } else if constexpr (std::is_same_v<T, AddRandomResponse>) {
        if (resp.which_payload != RpcResponse_rand_tag) return false;
        *out_msg = resp.payload.rand;
    } else if constexpr (std::is_same_v<T, ProcessFloatsResponse>) {
        if (resp.which_payload != RpcResponse_flt_tag) return false;
        *out_msg = resp.payload.flt;
    } else {
        static_assert(sizeof(T) == 0, "Unsupported message type in receive()");
    }

    std::free(buf);
    return true;
}


// Explicit template instantiations for the types used
template bool RpcClient::receive<SendMessageResponse>(uint32_t, SendMessageResponse*);
template bool RpcClient::receive<AddRandomResponse>(uint32_t, AddRandomResponse*);
template bool RpcClient::receive<ProcessFloatsResponse>(uint32_t, ProcessFloatsResponse*);
