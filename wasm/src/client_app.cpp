#include <cstdio>
#include <cstring>
#include "rpc_client.h"
#include "message.pb.h"

extern "C" {
    int64_t get_time_us();
    void send_rtt(uint32_t time_us);
    void send_total(uint32_t time_us, uint32_t count);
}

// Client that sends a nanopb message to a server and waits for a message response.
int main() {
    // std::printf("Client starting…\n");
    RpcClient client;

    // Build your message
    MyMessage msg = MyMessage_init_default;
    msg.id = 42;
    std::strncpy(msg.name, "hello from client", sizeof(msg.name) - 1);

    int64_t initial_time = get_time_us();
    int message_count = 1000;
    for (int i = 0; i < message_count; i++) {
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
    send_total(static_cast<uint32_t>(final_time - initial_time), message_count);

    return 0;
}
