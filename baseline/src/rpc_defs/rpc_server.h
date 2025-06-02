// rpc_server.h
#pragma once

#include "rpc/socket_communication.h"   // socket_listener, send_response_over_socket
#include <thread>
#include <atomic>
#include <vector>
#include <functional>

namespace rpc {

using request_handler_t = std::function<std::vector<uint8_t>(const std::vector<uint8_t>&)>;

class rpc_server {
public:
    rpc_server();
    ~rpc_server();

    // Start listening on message_port and dispatching to handler.
    void start(request_handler_t handler);

    // Stop listener and worker threads.
    void stop();

private:
    void worker_loop();

    std::thread _listener_thread;
    std::thread _worker_thread;
    std::atomic<bool> _running{false};
    request_handler_t _handler;
};

} // namespace rpc
