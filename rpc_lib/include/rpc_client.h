#ifndef RPC_CLIENT_H
#define RPC_CLIENT_H

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include "pb_encode.h"
#include "pb_decode.h"
#include "rpc_envelope.pb.h"   // RpcResponse, RpcResponse_fields
#include "message.pb.h"        // MyMessage, Ack, MyMessage_fields, Ack_fields

extern "C" {
    int32_t  send_rpcmessage(uint32_t offset, uint32_t length);
    int32_t  receive_rpcresponse(uint32_t offset, uint32_t length);
}

class RpcClient {
public:
    RpcClient() = default;
    ~RpcClient() = default;

    bool Send(const char* method,
              const void* message,
              const pb_msgdesc_t* fields);

    template<typename T>
    bool Receive(const pb_msgdesc_t* resp_fields, T* out_msg) {
        uint8_t* buf = (uint8_t*)std::malloc(512);
        if (!buf) return false;

        int32_t len = receive_rpcresponse((uint32_t)buf, 512);
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

        pb_istream_t payload_stream =
            pb_istream_from_buffer(resp.payload.bytes, resp.payload.size);
        bool ok = pb_decode(&payload_stream, resp_fields, out_msg);
        if (!ok) {
            std::fprintf(stderr, "Payload decode error: %s\n",
                         PB_GET_ERROR(&payload_stream));
        }

        std::free(buf);
        return ok;
    }

};

// ——— Explicit instantiation at global scope ———

// Tell the compiler to emit Receive<Ack> here:
template bool RpcClient::Receive<Ack>(const pb_msgdesc_t* resp_fields,
                                      Ack* out_msg);

#endif // RPC_CLIENT_H
