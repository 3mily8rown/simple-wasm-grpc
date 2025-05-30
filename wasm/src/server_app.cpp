#include <cstdio>
#include <cstring>
#include "rpc_server.h"
#include "message.pb.h"

int main() {
    std::printf("Server starting…\n");

    RpcServer server;

    // Register a handler for "SendMessage"
    server.RegisterHandler("SendMessage",
        [&](uint32_t req_id, const void* data, size_t size) -> bool {
            // Decode the incoming MyMessage
            MyMessage msg = MyMessage_init_zero;
            pb_istream_t pis = pb_istream_from_buffer((const uint8_t*)data, size);
            if (!pb_decode(&pis, MyMessage_fields, &msg)) {
                std::fprintf(stderr, "[Server] Failed to decode MyMessage: %s\n", PB_GET_ERROR(&pis));
                return false;
            }

            std::printf("[Server] Got id=%u name=\"%s\"\n", msg.id, msg.name);

            // Build an Ack
            Ack ack = Ack_init_zero;
            ack.success = true;
            std::snprintf(ack.info, sizeof(ack.info), "Hello back, %s!", msg.name);

            // Wrap in RpcResponse
            RpcResponse resp = RpcResponse_init_zero;
            resp.request_id = req_id;
            // encode Ack payload
            uint8_t payload[128];
            pb_ostream_t pos = pb_ostream_from_buffer(payload, sizeof(payload));
            pb_encode(&pos, Ack_fields, &ack);

            resp.payload.size = pos.bytes_written;
            std::memcpy(resp.payload.bytes, payload, pos.bytes_written);
            std::strncpy(resp.status, "OK", sizeof(resp.status)-1);

            // send it
            server.sendResponse(resp);
            return true;
        }
    );

    // Process one incoming call (or wrap in a loop)
    server.ProcessNextRequest();
    return 0;
}
