// main.cpp
#include <cstdio>
#include <cstring>
#include <vector>
#include <chrono>
#include <stdexcept>

#include "rpc_defs/rpc_client.h"      // your snake_case client
#include "message.pb.h"               // generated nanopb types for MyMessage/Ack
#include "rpc_envelope.pb.h"          // generated nanopb types for RpcRequest/RpcResponse
#include "pb_encode.h"
#include "pb_decode.h"

// Serialize any nanopb message into a vector<uint8_t>.
template<typename T>
std::vector<uint8_t> serialize_pb(const T* msg, const pb_msgdesc_t* desc) {
    size_t sz = 0;
    if (!pb_get_encoded_size(&sz, desc, msg)) {
        throw std::runtime_error("pb_get_encoded_size failed");
    }
    std::vector<uint8_t> buffer(sz);
    pb_ostream_t os = pb_ostream_from_buffer(buffer.data(), buffer.size());
    if (!pb_encode(&os, desc, msg)) {
        throw std::runtime_error("pb_encode failed");
    }
    return buffer;
}

// Deserialize into a nanopb message.
template<typename T>
bool deserialize_pb(const std::vector<uint8_t>& buf,
                    const pb_msgdesc_t* desc,
                    T* msg_out)
{
    pb_istream_t is = pb_istream_from_buffer(buf.data(), buf.size());
    return pb_decode(&is, desc, msg_out);
}

int main() {
    try {
        std::printf("Client starting…\n");

        rpc::rpc_client client;

        client.start_listener();  // Start the listener thread

        // 2) Build your MyMessage
        MyMessage mymsg = MyMessage_init_zero;
        mymsg.id = 42;
        std::strncpy(mymsg.name, "hello from client", sizeof(mymsg.name)-1);

        // 3) Serialize inner payload
        auto inner_buf = serialize_pb(&mymsg, &MyMessage_msg);

        // 4) Build the RpcRequest envelope
        RpcEnvelope req = RpcEnvelope_init_zero;
        req.request_id = 1;                             // arbitrary ID
        req.payload.size = inner_buf.size();
        std::memcpy(req.payload.bytes, inner_buf.data(), inner_buf.size());

        auto env_buf = serialize_pb(&req, &RpcEnvelope_msg);

        // 5) Send and time it
        auto t0 = std::chrono::high_resolution_clock::now();
        auto resp_buf = client.invoke(env_buf);
        // auto t1 = std::chrono::high_resolution_clock::now();

        // 6) Decode the RpcResponse envelope
        RpcResponse resp = RpcResponse_init_zero;
        if (!deserialize_pb(resp_buf, &RpcResponse_msg, &resp)) {
            std::fprintf(stderr, "Failed to decode RpcResponse\n");
            return 1;
        }

        // 7) Decode the inner Ack
        Ack ack = Ack_init_zero;
        std::vector<uint8_t> ack_buf(
            resp.payload.bytes,
            resp.payload.bytes + resp.payload.size
        );
        if (!deserialize_pb(ack_buf, &Ack_msg, &ack)) {
            std::fprintf(stderr, "Failed to decode Ack\n");
            return 1;
        }
        // here to mirror wasm codes timing
        auto t1 = std::chrono::high_resolution_clock::now();
        uint32_t rtt_us = static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count()
        );

        // 8) Print results
        std::printf("[Client] Ack success=%d info=\"%s\"\n",
                    ack.success, ack.info);
        std::printf("[METRICS] RTT = %u μs\n", rtt_us);

        client.stop_listener();
        return 0;
    } catch (...) {
        std::fprintf(stderr, "Unhandled non‐std exception\n");
        return 1;
  }
}
