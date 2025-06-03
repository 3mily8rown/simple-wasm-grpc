#include "rpc_server.h"
#include <string>
#include <vector>
#include <numeric> // for std::accumulate

// Simple business logic functions

std::string greet(int32_t id, std::string name) {
    return "Hello back, " + name + "!";
}

int32_t add_constant(int32_t num) {
    return num + 42;
}

float sum_floats(std::vector<float> nums) {
    return std::accumulate(nums.begin(), nums.end(), 0.0f);
}

int main() {
    RpcServer server;

    // Register clean business logic handlers
    server.registerFunction(RpcEnvelope_msg_tag, greet);
    server.registerFunction(RpcEnvelope_rand_tag, add_constant);
    server.registerFunction(RpcEnvelope_flt_tag, sum_floats);

    // Process requests forever
    while (server.ProcessNextRequest()) {}

    return 0;
}
