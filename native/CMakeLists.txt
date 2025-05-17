cmake_minimum_required(VERSION 3.8)

project(NativeRPC C CXX)

# --- pull in the folder where the wasm step dropped the stubs ---
if(NOT DEFINED IMPORTS_DIR)
  message(FATAL_ERROR "IMPORTS_DIR must be set to your <install>/generated directory")
endif()

include (FetchContent)

file(GLOB_RECURSE SOURCES
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
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

# builds wamr, creating wamrlib static library and links to public interface of wamrlib
include (${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)
add_library(wamrlib "${WAMR_RUNTIME_LIB_SOURCE}")
target_include_directories(wamrlib PUBLIC
        ${PLATFORM_SHARED_DIR}
)

# ----------------------------------
# Targets
# ----------------------------------

target_include_directories(wasm_host PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${wamr_ext_SOURCE_DIR}/core/iwasm/include"
    "${IMPORTS_DIR}"
)

# Link the prebuilt libraries
target_link_libraries(wasm_host
        wamrlib
        pthread
        m
        dl
)

install(TARGETS wasm_host
        RUNTIME DESTINATION bin)

