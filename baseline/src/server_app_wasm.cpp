// rpc_server.cpp
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <arpa/inet.h>

#include "rpc/socket_communication.h"
#include "rpc/message_queue.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "rpc_envelope.pb.h"
#include "message.pb.h"

class RpcServer {
public:
    static constexpr uint16_t kMessagePort  = 12345;
    static constexpr uint16_t kResponsePort = 12346;

    RpcServer() {
        listener_thread_ = std::thread(&RpcServer::ListenLoop, this);
        std::cout << "[Server] Listener started on port " << kMessagePort << "\n";
    }
    ~RpcServer() {
        if (listener_thread_.joinable()) listener_thread_.join();
    }
    void Run() { HandleLoop(); }

private:
    std::thread listener_thread_;

    void ListenLoop() {
        socket_listener(kMessagePort, INADDR_ANY);
    }

    void HandleLoop() {
        uint8_t buf[1024];

        while (true) {
            // 1) Capture the real length of the queued frame
            uint32_t len = dequeue_message(buf, sizeof(buf));
            // (blocks until data arrives)

            // 2) Decode exactly `len` bytes, not sizeof(buf)
            pb_istream_t in = pb_istream_from_buffer(buf, len);
            RpcEnvelope env = RpcEnvelope_init_zero;
            if (!pb_decode(&in, RpcEnvelope_fields, &env)) {
                std::cerr << "[Server] Envelope decode failed: "
                          << PB_GET_ERROR(&in) << "\n";
                continue;
            }

            // 3) Decode inner MyMessage
            MyMessage msg = MyMessage_init_zero;
            pb_istream_t pin =
                pb_istream_from_buffer(env.payload.bytes,
                                       env.payload.size);
            if (!pb_decode(&pin, MyMessage_fields, &msg)) {
                std::cerr << "[Server] MyMessage decode failed: "
                          << PB_GET_ERROR(&pin) << "\n";
                continue;
            }

            std::cout << "[Server] Got id=" << msg.id
                      << " name=\"" << msg.name << "\"\n";

            // 4) Build Ack
            Ack ack = Ack_init_zero;
            ack.success = true;
            snprintf(ack.info, sizeof(ack.info),
                     "Hello, %s", msg.name);

            // 5) Encode and send RpcResponse (plain protobuf)
            uint8_t payload_buf[128];
            pb_ostream_t pout =
                pb_ostream_from_buffer(payload_buf, sizeof(payload_buf));
            pb_encode(&pout, Ack_fields, &ack);

            RpcResponse resp = RpcResponse_init_zero;
            resp.request_id = env.request_id;
            size_t L = pout.bytes_written;
            memcpy(resp.payload.bytes, payload_buf, L);
            resp.payload.size = L;
            strncpy(resp.status, "OK", sizeof(resp.status)-1);

            uint8_t outbuf[256];
            pb_ostream_t rout =
                pb_ostream_from_buffer(outbuf, sizeof(outbuf));
            pb_encode(&rout, RpcResponse_fields, &resp);

            send_response_over_socket(outbuf, rout.bytes_written);
        }
    }
};

int main() {
    RpcServer server;
    server.Run();
    return 0;
}
