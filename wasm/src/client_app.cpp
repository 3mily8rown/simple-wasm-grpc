#include <cstdio>
#include <cstring>
#include <algorithm>
#include "rpc_client.h"
#include "message.pb.h"
#include "rpc_envelope.pb.h"


extern "C" {
    int64_t get_time_us();
    void send_rtt(uint32_t time_us);
    void send_total(uint32_t time_us, uint32_t count);
}

int send_x_messages(int count) {
    RpcClient client;
    SendMessage msg = SendMessage_init_zero;
    msg.id = 42;
    std::strncpy(msg.name, "hello from client", sizeof(msg.name) - 1);

    int64_t initial_time = get_time_us();
    for (int i = 0; i < count; i++) {
        int64_t t0 = get_time_us();

        if (!client.Send(i, RpcEnvelope_msg_tag, &msg)) {
            std::fprintf(stderr, "Failed to Send\n");
            return 1;
        }

        SendMessageResponse ack = SendMessageResponse_init_zero;
        if (!client.Receive<SendMessageResponse>(RpcResponse_msg_tag, &ack)) {
            std::fprintf(stderr, "Failed to get response\n");
            return 1;
        }

        int64_t t1 = get_time_us();
        uint32_t rtt = static_cast<uint32_t>(t1 - t0);
        send_rtt(rtt);
    }

    int64_t final_time = get_time_us();
    send_total(static_cast<uint32_t>(final_time - initial_time), count);
    return 0;
}

int send_varied_messages(int count) {
    RpcClient client;
    SendMessage msg = SendMessage_init_zero;
    msg.id = 42;

    for (int i = 8; i < 2048; i *= 2) {
        int fill_len = std::min(i, static_cast<int>(sizeof(msg.name)) - 1);
        std::memset(msg.name, 'A', fill_len);
        msg.name[fill_len] = '\0';

        int64_t initial_time = get_time_us();
        for (int j = 0; j < count; j++) {
            int64_t t0 = get_time_us();

            if (!client.Send(j, RpcEnvelope_msg_tag, &msg)) {
                std::fprintf(stderr, "Failed to Send\n");
                return 1;
            }

            SendMessageResponse ack = SendMessageResponse_init_zero;
            if (!client.Receive<SendMessageResponse>(RpcResponse_msg_tag, &ack)) {
                std::fprintf(stderr, "Failed to get response\n");
                return 1;
            }

            int64_t t1 = get_time_us();
            uint32_t rtt = static_cast<uint32_t>(t1 - t0);
            send_rtt(rtt);
        }

        int64_t final_time = get_time_us();
        send_total(static_cast<uint32_t>(final_time - initial_time), count);
    }

    return 0;
}

int main() {
    return send_x_messages(1);
    // return send_varied_messages(10);
}
