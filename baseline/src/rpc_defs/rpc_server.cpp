// rpc_server.cpp
#include "rpc_server.h"
#include "rpc/message_queue.h"          // dequeue_message
#include "rpc/socket_communication.h"   // socket_listener(), send_response_over_socket()
#include <chrono>

namespace rpc {

rpc_server::rpc_server() = default;
rpc_server::~rpc_server() { stop(); }

void rpc_server::start(request_handler_t handler) {
    if (_running) return;
    _handler = std::move(handler);
    _running = true;

    // This will read from message_port and call queue_message() internally.
    _listener_thread = std::thread([](){
        socket_listener();
    });

    // This pulls from the queue, invokes handler, then replies.
    _worker_thread = std::thread(&rpc_server::worker_loop, this);
}

void rpc_server::stop() {
    if (!_running) return;
    _running = false;
    if (_listener_thread.joinable()) _listener_thread.join();
    if (_worker_thread.joinable())  _worker_thread.join();
}

void rpc_server::worker_loop() {
    std::vector<uint8_t> buffer(4096);

    while (_running) {
        // Block until a request is available
        uint32_t n = dequeue_message(buffer.data(), static_cast<uint32_t>(buffer.size()));
        if (n > 0) {
            buffer.resize(n);
            // Process the request
            auto response = _handler(buffer);
            // Enqueue and send back
            queue_response(response.data(), static_cast<uint32_t>(response.size()));
            send_response_over_socket(response.data(), static_cast<uint32_t>(response.size()));
            buffer.resize(4096);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

} // namespace rpc
