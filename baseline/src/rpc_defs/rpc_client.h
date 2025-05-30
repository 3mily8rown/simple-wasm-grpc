// rpc_client.h
#pragma once
#include <vector>
#include <thread>
#include <atomic>

namespace rpc {
  class rpc_client {
  public:
    rpc_client();
    ~rpc_client();

    // Fire-and-wait
    void send_request(const std::vector<uint8_t>& request);
    std::vector<uint8_t> invoke(const std::vector<uint8_t>& request);

    // Just keep the listener thread alive
    void start_listener();
    void stop_listener();

  private:
    void listener_loop();
    int   _listen_fd = -1;
    std::thread   _listener_thread;
    std::atomic<bool> _listening{false};
  };
}
