#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstdlib>
#include <string>
#include <stdexcept>

namespace Config {
    // helper: get env, or throw if unset
    inline std::string get(const char* var) {
        const char* val = std::getenv(var);
        if (!val) {
            throw std::runtime_error(std::string("Required environment variable not set: ") + var);
        }
        return std::string(val);
    }

    // now your project-wide constants (no defaults):
    inline const std::string WASM_OUT = get("WASM_OUT");

    
}

#endif // CONFIG_HPP
