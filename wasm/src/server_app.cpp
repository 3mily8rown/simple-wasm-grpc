#include <cstdio>
#include <cstring>
#include "rpc_server.h"
#include "message.pb.h"
#include "rpc_envelope.pb.h"


int main() {
    RpcServer server;

    // Register handler for RpcEnvelope_msg_tag (SendMessage)
    server.RegisterHandler(RpcEnvelope_msg_tag,
        [&](uint32_t req_id, const RpcEnvelope& env) -> bool {
            const SendMessage& msg = env.payload.msg;

            // Build a response
            SendMessageResponse ack = SendMessageResponse_init_zero;
            std::snprintf(ack.info, sizeof(ack.info), "Hello back, %s!", msg.name);

            // Wrap in RpcResponse
            RpcResponse resp = RpcResponse_init_zero;
            resp.request_id = req_id;
            resp.which_payload = RpcResponse_msg_tag;
            resp.payload.msg = ack;
            resp.status = true; // assuming you kept a `bool status` field

            server.sendResponse(resp);
            return true;
        }
    );

    // Process requests in a loop
    while (server.ProcessNextRequest()) {
        // optional sleep/yield
    }

    return 0;
}