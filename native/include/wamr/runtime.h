#pragma once
#include <mutex>
#include <wasm_export.h>

namespace wamr {

class Runtime {
public:
    Runtime();
    ~Runtime();
private:
    inline static bool   initialised_ = false;
    inline static std::mutex mu_;
    inline static char   heap_[512 * 1024];
};

}
