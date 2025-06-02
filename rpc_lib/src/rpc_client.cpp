#include "rpc_client.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

bool RpcClient::Send(uint32_t request_id,
                     uint32_t payload_tag,
                     const void* message)
{
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

    // Encode the envelope
    uint8_t buf[1024];
    pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(buf));
    if (!pb_encode(&stream, RpcEnvelope_fields, &env)) {
        std::fprintf(stderr, "RpcEnvelope encode error: %s\n", PB_GET_ERROR(&stream));
        return false;
    }

    // Allocate in WASM heap and send
    uint32_t len = (uint32_t)stream.bytes_written;
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
