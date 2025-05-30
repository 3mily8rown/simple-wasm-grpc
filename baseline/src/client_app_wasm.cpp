// rpc_client.cpp
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <arpa/inet.h>

#include "rpc/socket_communication.h"
#include "rpc/message_queue.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "rpc_envelope.pb.h"
#include "message.pb.h"

static constexpr uint16_t kMessagePort  = 12345;
static constexpr uint16_t kResponsePort = 12346;

class RpcClient {
public:
    RpcClient() {
        resp_thread_ = std::thread(&RpcClient::ListenResponses, this);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    ~RpcClient() {
        if (resp_thread_.joinable()) resp_thread_.join();
    }

    void Send(const char* method, const void* message, const pb_msgdesc_t* fields) {
        // Encode inner MyMessage
        uint8_t payload_buf[128];
        pb_ostream_t pout = pb_ostream_from_buffer(payload_buf, sizeof(payload_buf));
        pb_encode(&pout, fields, message);

        // Build and encode RpcEnvelope (plain)
        RpcEnvelope env = RpcEnvelope_init_zero;
        strncpy(env.method, method, sizeof(env.method)-1);
        memcpy(env.payload.bytes, payload_buf, pout.bytes_written);
        env.payload.size = pout.bytes_written;

        uint8_t env_buf[256];
        pb_ostream_t eout = pb_ostream_from_buffer(env_buf, sizeof(env_buf));
        pb_encode(&eout, RpcEnvelope_fields, &env);

        // Send exactly those bytes
        send_over_socket(env_buf, eout.bytes_written, get_server_ip(), kMessagePort);
    }

    Ack ReceiveAck() {
        uint8_t buf[1024];
        uint32_t len = dequeue_response(buf, sizeof(buf));  // real length

        // Decode RpcResponse (plain)
        pb_istream_t rin = pb_istream_from_buffer(buf, len);
        RpcResponse resp = RpcResponse_init_zero;
        pb_decode(&rin, RpcResponse_fields, &resp);

        // Decode inner Ack
        Ack ack = Ack_init_zero;
        pb_istream_t ain = pb_istream_from_buffer(resp.payload.bytes, resp.payload.size);
        pb_decode(&ain, Ack_fields, &ack);
        return ack;
    }

private:
    std::thread resp_thread_;
    void ListenResponses() {
        socket_response_listener(kResponsePort, INADDR_ANY);
    }
};

int main() {
    RpcClient client;
    MyMessage msg = MyMessage_init_default;
    msg.id = 42;
    strcpy(msg.name, "hello from client");

    client.Send("SendMessage", &msg, MyMessage_fields);
    Ack ack = client.ReceiveAck();
    std::cout << "Ack: success=" << ack.success
              << " info=\"" << ack.info << "\"\n";
    return 0;
}
