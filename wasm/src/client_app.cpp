#include <cstdio>
#include <cstring>
#include "rpc_client.h"
#include "message.pb.h"
#include <algorithm>

extern "C" {
    int64_t get_time_us();
    void send_rtt(uint32_t time_us);
    void send_total(uint32_t time_us, uint32_t count);
}

int send_x_messages(int count) {
    // std::printf("Client starting…\n");
    RpcClient client;

    // Build your message
    MyMessage msg = MyMessage_init_default;
    msg.id = 42;
    std::strncpy(msg.name, "hello from client", sizeof(msg.name) - 1);
    
    int64_t initial_time = get_time_us();
    for (int i = 0; i < count; i++) {
        // for performance measurement
        int64_t t0 = get_time_us();
        
        // Send
        if (!client.Send("SendMessage", &msg, MyMessage_fields)) {
            std::fprintf(stderr, "Failed to Send\n");
            fflush(stderr);
            return 1;
        }

        // Receive
        Ack ack = Ack_init_zero;
        if (!client.Receive<Ack>(Ack_fields, &ack)) {
            std::fprintf(stderr, "Failed to get Ack\n");
            fflush(stderr);
            std::exit(1);
            // return 1;
        }
        int64_t t1 = get_time_us();

        // std::printf("[Client] Ack success=%d info=\"%s\"\n", ack.success, ack.info);
        uint32_t rtt = static_cast<uint32_t>(t1 - t0);
        // Send the RTT back to the host
        // std::printf("[Client] RTT: %u us\n", rtt);
        // fflush(stdout);
        send_rtt(rtt);
    }

    int64_t final_time = get_time_us();
    send_total(static_cast<uint32_t>(final_time - initial_time), count);

    return 0;
}

int send_varied_messages(int count) {
    // std::printf("Client starting…\n");
    RpcClient client;

    // Build your message
    MyMessage msg = MyMessage_init_default;
    msg.id = 42;
    std::strncpy(msg.name, "hello from client", sizeof(msg.name) - 1);

    // Vary size of the message
    for (int i = 8; i < 2048; i *= 2) {
        int fill_len = std::min(i, (int)sizeof(msg.name) - 1);
        std::memset(msg.name, 'A', fill_len);
        msg.name[fill_len] = '\0';  // Null-terminate
    
    
        int64_t initial_time = get_time_us();
        for (int i = 0; i < count; i++) {
            // for performance measurement
            int64_t t0 = get_time_us();
            
            // Send
            if (!client.Send("SendMessage", &msg, MyMessage_fields)) {
                std::fprintf(stderr, "Failed to Send\n");
                fflush(stderr);
                return 1;
            }

            // Receive
            Ack ack = Ack_init_zero;
            if (!client.Receive<Ack>(Ack_fields, &ack)) {
                std::fprintf(stderr, "Failed to get Ack\n");
                fflush(stderr);
                std::exit(1);
                // return 1;
            }
            int64_t t1 = get_time_us();

            // std::printf("[Client] Ack success=%d info=\"%s\"\n", ack.success, ack.info);
            uint32_t rtt = static_cast<uint32_t>(t1 - t0);
            // Send the RTT back to the host
            // std::printf("[Client] RTT: %u us\n", rtt);
            // fflush(stdout);
            send_rtt(rtt);
        }

        int64_t final_time = get_time_us();
        send_total(static_cast<uint32_t>(final_time - initial_time), count);

    }
    return 0;
}

// TODO: a function that just has fixed sizes? a function that doesnt have a nanopb internal message?

int main() {
    // return send_x_messages(1000);
    return send_varied_messages(10);
}
