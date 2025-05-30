// main.cpp
#include <cstdio>
#include <cstring>
#include <thread>
#include "rpc_defs/rpc_server.h"
#include "message.pb.h"        // nanopb-generated
#include "rpc_envelope.pb.h"   // your RpcResponse envelope
#include "pb_encode.h"
#include "pb_decode.h"

int main() {
    std::printf("Server starting…\n");

    rpc::rpc_server server;

    server.start(
        [&](const std::vector<uint8_t>& req_buf) -> std::vector<uint8_t>
        {
            // === 1) Decode incoming RpcRequest envelope ===
            RpcEnvelope request = RpcEnvelope_init_zero;
            pb_istream_t  istream = pb_istream_from_buffer(
                req_buf.data(), req_buf.size()
            );
            if (!pb_decode(&istream, RpcEnvelope_fields, &request)) {
                std::fprintf(stderr,
                    "[Server] Failed to decode RpcRequest: %s\n",
                    PB_GET_ERROR(&istream)
                );
                return {};  // empty = error/no reply
            }

            // === 2) Decode the nested MyMessage payload ===
            MyMessage msg = MyMessage_init_zero;
            pb_istream_t  pis = pb_istream_from_buffer(
                request.payload.bytes,
                request.payload.size
            );
            if (!pb_decode(&pis, MyMessage_fields, &msg)) {
                std::fprintf(stderr,
                    "[Server] Failed to decode MyMessage: %s\n",
                    PB_GET_ERROR(&pis)
                );
                return {};
            }

            std::printf("[Server] Got id=%u name=\"%s\"\n",
                        msg.id, msg.name);

            // === 3) Build your Ack ===
            Ack ack = Ack_init_zero;
            ack.success = true;
            std::snprintf(ack.info, sizeof(ack.info),
                          "Hello back, %s!", msg.name);

            // === 4) Wrap into RpcResponse envelope ===
            RpcResponse response = RpcResponse_init_zero;
            response.request_id = request.request_id;
            // serialize the Ack payload
            pb_ostream_t pos = pb_ostream_from_buffer(
                response.payload.bytes,
                sizeof(response.payload.bytes)
            );
            if (!pb_encode(&pos, Ack_fields, &ack)) {
                std::fprintf(stderr,
                    "[Server] Failed to encode Ack: %s\n",
                    PB_GET_ERROR(&pos)
                );
                return {};
            }
            response.payload.size = pos.bytes_written;
            std::strncpy(response.status, "OK",
                         sizeof(response.status)-1);

            // === 5) Serialize the full RpcResponse ===
            std::vector<uint8_t> out;
            out.resize( /* envelope size upper-bound */ 512 );
            pb_ostream_t ostream = pb_ostream_from_buffer(
                out.data(), out.size()
            );
            if (!pb_encode(&ostream, RpcResponse_fields, &response)) {
                std::fprintf(stderr,
                    "[Server] Failed to encode RpcResponse: %s\n",
                    PB_GET_ERROR(&ostream)
                );
                return {};
            }
            out.resize(ostream.bytes_written);
            return out;
        }
    );

    // Keep the server alive
    std::printf("Server running. Press Ctrl+C to quit.\n");
    while (true) {
        std::this_thread::sleep_for(std::chrono::minutes(1));
    }
    server.stop();

    return 0;
}
