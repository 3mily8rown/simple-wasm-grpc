#include "rpc_client.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

bool RpcClient::Send(const char* method,
                     const void* message,
                     const pb_msgdesc_t* fields)
{
    // 1) Encode the payload
    uint8_t payload_buf[128];
    pb_ostream_t payload_stream = pb_ostream_from_buffer(payload_buf, sizeof(payload_buf));
    if (!pb_encode(&payload_stream, fields, message)) {
        std::fprintf(stderr, "Payload encode error: %s\n", PB_GET_ERROR(&payload_stream));
        return false;
    }

    // 2) Wrap in RpcEnvelope
    RpcEnvelope env = RpcEnvelope_init_zero;
    std::strncpy(env.method, method, sizeof(env.method) - 1);
    std::memcpy(env.payload.bytes, payload_buf, payload_stream.bytes_written);
    env.payload.size = payload_stream.bytes_written;

    // 3) Encode the envelope
    uint8_t envelope_buf[256];
    pb_ostream_t env_stream = pb_ostream_from_buffer(envelope_buf, sizeof(envelope_buf));
    if (!pb_encode(&env_stream, RpcEnvelope_fields, &env)) {
        std::fprintf(stderr, "Envelope encode error: %s\n", PB_GET_ERROR(&env_stream));
        return false;
    }

    // 4) Allocate in WASM heap and send
    uint32_t len = (uint32_t)env_stream.bytes_written;
    uint8_t* wasm_buf = (uint8_t*)std::malloc(len);
    if (!wasm_buf) {
        std::fprintf(stderr, "malloc failed\n");
        return false;
    }
    std::memcpy(wasm_buf, envelope_buf, len);
    int32_t rc = send_rpcmessage((uint32_t)wasm_buf, len);
    std::free(wasm_buf);

    return rc >= 0;
}

// template<typename T>
// bool RpcClient::Receive(const pb_msgdesc_t* resp_fields, T* out_msg)
// {
//     // 1) Allocate a buffer to read into
//     uint8_t* buf = (uint8_t*)std::malloc(512);
//     if (!buf) return false;

//     int32_t len = receive_rpcresponse((uint32_t)buf, 512);
//     if (len <= 0) {
//         std::free(buf);
//         return false;
//     }

//     // 2) Decode the outer RpcResponse
//     RpcResponse resp = RpcResponse_init_zero;
//     pb_istream_t stream = pb_istream_from_buffer(buf, len);
//     if (!pb_decode(&stream, RpcResponse_fields, &resp)) {
//         std::fprintf(stderr, "Response decode error: %s\n", PB_GET_ERROR(&stream));
//         std::free(buf);
//         return false;
//     }

//     // 3) Decode the inner payload into the user-provided message
//     pb_istream_t payload_stream = pb_istream_from_buffer(resp.payload.bytes, resp.payload.size);
//     bool ok = pb_decode(&payload_stream, resp_fields, out_msg);
//     if (!ok) {
//         std::fprintf(stderr, "Payload decode error: %s\n", PB_GET_ERROR(&payload_stream));
//     }

//     std::free(buf);
//     return ok;
// }

// Explicit instantiation for your Ack type:
// template bool RpcClient::Receive<Ack>(const pb_msgdesc_t*, Ack*);

