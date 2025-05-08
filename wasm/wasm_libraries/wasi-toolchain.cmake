# wasi-toolchain.cmake
set(CMAKE_SYSTEM_NAME WASI)
set(CMAKE_SYSTEM_VERSION 1)

set(WASI_SDK_PATH "/home/eb/fyp/wasi-sdk-25.0-x86_64-linux")

set(CMAKE_C_COMPILER "${WASI_SDK_PATH}/bin/clang")
set(CMAKE_CXX_COMPILER "${WASI_SDK_PATH}/bin/clang++")
set(CMAKE_AR "${WASI_SDK_PATH}/bin/llvm-ar")
set(CMAKE_RANLIB "${WASI_SDK_PATH}/bin/llvm-ranlib")

set(CMAKE_C_COMPILER_TARGET "wasm32-wasi")
set(CMAKE_CXX_COMPILER_TARGET "wasm32-wasi")
set(CMAKE_SYSROOT "${WASI_SDK_PATH}/share/wasi-sysroot")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/Platform")

# Avoid trying to use pthreads with WASI
set(THREADS_PREFER_PTHREAD_FLAG OFF CACHE BOOL "Don't prefer pthreads")
set(CMAKE_USE_PTHREADS_INIT OFF CACHE BOOL "Don't use pthreads")
set(CMAKE_HAVE_THREADS_LIBRARY 0 CACHE INTERNAL "No threads library")
set(CMAKE_THREAD_PREFER_PTHREAD OFF CACHE BOOL "Don't prefer pthreads")
set(THREADS_FOUND FALSE CACHE BOOL "Threads not available for WASI")


set(CMAKE_SYSTEM_PROCESSOR wasm32)
set(CMAKE_C_FLAGS "--sysroot=${WASI_SDK_PATH}/share/wasi-sysroot")
set(CMAKE_CXX_FLAGS "--sysroot=${WASI_SDK_PATH}/share/wasi-sysroot")

