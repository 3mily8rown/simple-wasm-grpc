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

    int64_t initial_time = get_time_us();
    for (int i = 0; i < count; i++) {
        int64_t t0 = get_time_us();

        std::string ack = client.sendMessage(i, "hello from client");
        if (ack.empty()) {
            std::fprintf(stderr, "Failed to send message\n");
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

    for (int i = 8; i < 2048; i *= 2) {
        char name_buf[sizeof(SendMessage::name)] = {};
        int fill_len = std::min(i, static_cast<int>(sizeof(name_buf)) - 1);
        std::memset(name_buf, 'A', fill_len);
        name_buf[fill_len] = '\0';

        int64_t initial_time = get_time_us();

        for (int j = 0; j < count; j++) {
            int64_t t0 = get_time_us();

            std::string ack = client.sendMessage(j, name_buf);
            if (ack.empty()) {
                std::fprintf(stderr, "Failed to send message\n");
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
