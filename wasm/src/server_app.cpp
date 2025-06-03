#include <cstdio>
#include <cstring>
#include "rpc_server.h"
#include "message.pb.h"
#include "rpc_envelope.pb.h"


int main() {
    RpcServer server;

    server.RegisterHandler(RpcEnvelope_msg_tag,
        [&](uint32_t req_id, const RpcEnvelope& env) -> bool {
            const SendMessage& msg = env.payload.msg;

            SendMessageResponse resp = SendMessageResponse_init_zero;
            std::snprintf(resp.info, sizeof(resp.info), "Hello back, %s!", msg.name);

            RpcResponse out = RpcResponse_init_zero;
            out.request_id = req_id;
            out.which_payload = RpcResponse_msg_tag;
            out.payload.msg = resp;
            out.status = true;

            server.sendResponse(out);
            return true;
        }
    );

    server.RegisterHandler(RpcEnvelope_rand_tag,
        [&](uint32_t req_id, const RpcEnvelope& env) -> bool {
            const AddRandom& rand = env.payload.rand;

            AddRandomResponse resp = AddRandomResponse_init_zero;
            resp.sum = rand.num + 42; // do something

            RpcResponse out = RpcResponse_init_zero;
            out.request_id = req_id;
            out.which_payload = RpcResponse_rand_tag;
            out.payload.rand = resp;
            out.status = true;

            server.sendResponse(out);
            return true;
        }
    );

    server.RegisterHandler(RpcEnvelope_flt_tag,
        [&](uint32_t req_id, const RpcEnvelope& env) -> bool {
            const ProcessFloats& flt = env.payload.flt;

            ProcessFloatsResponse resp = ProcessFloatsResponse_init_zero;
            for (int i = 0; i < flt.num_count; ++i) {
                resp.sum += flt.num[i];
            }

            RpcResponse out = RpcResponse_init_zero;
            out.request_id = req_id;
            out.which_payload = RpcResponse_flt_tag;
            out.payload.flt = resp;
            out.status = true;

            server.sendResponse(out);
            return true;
        }
    );


    // Process requests in a loop
    while (server.ProcessNextRequest()) {
        // optional sleep/yield
    }

    return 0;
}