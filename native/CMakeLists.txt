cmake_minimum_required(VERSION 3.8)

project(NativeGRPC C CXX)

include (FetchContent)

set(PROTO_DIR $ENV{PROTO_DIR})
set(GEN_DIR $ENV{GEN_DIR})
set(PROTOBUF $ENV{PROTOBUF})

file(GLOB_RECURSE SOURCES
     "${CMAKE_SOURCE_DIR}/src/*.cpp"
     "${PROTO_DIR}/generated_full/*.cc"
)

# ----------------------------------
# Executable
# ----------------------------------

# Build the wasm_host executable
add_executable(wasm_host 
  ${SOURCES}
)

# ------------------------------
# Add WAMR support
# ------------------------------

# downloading WAMR
FetchContent_Declare(
        wamr_ext
        GIT_REPOSITORY "https://github.com/bytecodealliance/wasm-micro-runtime"
        GIT_TAG "4e50d2191ca8f177ad03a9d80eebc44b59a932db"
)
FetchContent_MakeAvailable(wamr_ext)

# extracts path downloaded WAMR to
FetchContent_GetProperties(wamr_ext SOURCE_DIR WAMR_ROOT_DIR)
message(STATUS WAMR_ROOT_DIR ${WAMR_ROOT_DIR})

# configure which features of WAMR to include
set(WAMR_BUILD_PLATFORM "linux")
set(WAMR_BUILD_TARGET X86_64)
set(WAMR_BUILD_SPEC_TEST 0)
set(WAMR_BUILD_INTERP 1)   # builds interpreter version not AoT/JIT
set(WAMR_BUILD_AOT 0)
set(WAMR_BUILD_JIT 0)
set(WAMR_BUILD_LAZY_JIT 0)
set(WAMR_BUILD_LIBC_BUILTIN 1)
set(WAMR_BUILD_LIBC_WASI 1)   # WASI support for syscalls
set(WAMR_BUILD_LIB_PTHREAD 0)
set(WAMR_BUILD_REF_TYPES 1)

# builds wamr, creating wamrlib static library and links to public interface of wamrlib
include (${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)
add_library(wamrlib "${WAMR_RUNTIME_LIB_SOURCE}")
target_include_directories(wamrlib PUBLIC
        ${PLATFORM_SHARED_DIR}
)

# ----------------------------------
# Protobuf & Abseil
# ----------------------------------

add_subdirectory(third_party/abseil-cpp)

target_include_directories(wasm_host PRIVATE
    "${PROTO_DIR}/generated_full"
    include
    include/utils
    "${GEN_DIR}"
    "${wamr_ext_SOURCE_DIR}/core/iwasm/include"
    "${PROTOBUF}/src"
)

# Link the prebuilt libraries
target_link_libraries(wasm_host
        wamrlib
        pthread
        m
        dl
        absl::base
        ${PROTOBUF}/build/libprotobuf.a
)
