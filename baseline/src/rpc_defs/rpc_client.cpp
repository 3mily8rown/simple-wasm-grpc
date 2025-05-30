// rpc_client.cpp
#include "rpc_client.h"
#include "rpc/socket_communication.h"   // socket_response_listener()
#include "rpc/message_queue.h"          // dequeue_response(), queue_message()
#include <chrono>
#include <thread>
#include <cstdio>

namespace rpc {

rpc_client::rpc_client() = default;

rpc_client::~rpc_client() {
    stop_listener();
}

void rpc_client::send_request(const std::vector<uint8_t>& request) {
    queue_message(request.data(), static_cast<uint32_t>(request.size()));
    send_over_socket(request.data(), static_cast<uint32_t>(request.size()));
}

std::vector<uint8_t> rpc_client::invoke(const std::vector<uint8_t>& request) {
    send_request(request);

    std::vector<uint8_t> buffer(4096);
    while (true) {
        uint32_t n = dequeue_response(buffer.data(), static_cast<uint32_t>(buffer.size()));
        if (n > 0) {
            buffer.resize(n);
            return buffer;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void rpc_client::start_listener() {
    if (_listening) return;
    _listening = true;
    // Kick off your existing socket_response_listener() on a detached thread
    std::thread([](){
        socket_response_listener();  // defaults to response_port
    }).detach();
}


void rpc_client::stop_listener() {
    if (!_listening) return;
    _listening = false;
    // You’ll need to make socket_response_listener() exit when !_listening,
    // or wrap it yourself. For now, assume it checks _listening.
    if (_listener_thread.joinable())
        _listener_thread.join();
}

void rpc_client::listener_loop() {
    // This *blocks* in accept/read and queues every incoming packet:
    socket_response_listener(/*port=*/response_port, /*ip=*/INADDR_NONE);
}

} // namespace rpc
