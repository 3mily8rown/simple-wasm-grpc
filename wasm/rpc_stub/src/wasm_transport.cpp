#include <pw_status/status.h>
#include <pw_rpc/channel.h>
#include <span>

extern "C" {
    void host_send(uint32_t off, uint32_t len);
    uint32_t host_recv(uint32_t off, uint32_t max_len);
}

struct WasmTransport : public pw::rpc::ChannelOutput {
  pw::Status Send(std::span<const std::byte> data) override {
    host_send(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    return pw::OkStatus();
  }
};
