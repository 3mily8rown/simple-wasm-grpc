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

