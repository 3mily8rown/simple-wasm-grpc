cmake_minimum_required(VERSION 3.15)
project(super LANGUAGES NONE)
include(ExternalProject)

# -----------------------------------------------------------------
# host_tools (protoc) (if one day stop the exports python thing then can just put this in native, and run wasm after it)
# -----------------------------------------------------------------
# ExternalProject_Add(host_tools
#   SOURCE_DIR ${CMAKE_SOURCE_DIR}/host_tools
#   INSTALL_DIR ${CMAKE_BINARY_DIR}/install
#   CMAKE_ARGS 
#     -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
#     -DCMAKE_C_COMPILER=/usr/bin/clang
#     -DCMAKE_CXX_COMPILER=/usr/bin/clang++

# )
find_program(SYSTEM_PROTOC protoc)
if(NOT SYSTEM_PROTOC)
  message(FATAL_ERROR "protoc not found; please install protobuf-compiler")
endif()

# -----------------------------------------------------------------
# wasm (needs protoc from step 1)
# -----------------------------------------------------------------
ExternalProject_Add(wasm
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/wasm
  BINARY_DIR ${CMAKE_BINARY_DIR}/wasm-prefix/src/wasm-build
  INSTALL_DIR ${CMAKE_BINARY_DIR}/install

  CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE=/home/eb/fyp/wasi-sdk-25.0-x86_64-linux/share/cmake/wasi-sdk.cmake
        -DPROTOC_EXECUTABLE=${SYSTEM_PROTOC}
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DPROTO_DIR=${CMAKE_SOURCE_DIR}/wasm/proto_messages
        -DIMPORTS_DIR=<INSTALL_DIR>/generated
        -DSCRIPT_DIR=${CMAKE_SOURCE_DIR}/host_tools/scripts

)

# -----------------------------------------------------------------
# native (needs imports.h from step 2)
# -----------------------------------------------------------------
ExternalProject_Add(native
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/native
  INSTALL_DIR ${CMAKE_BINARY_DIR}/install #am i installing?
  CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DIMPORTS_DIR=<INSTALL_DIR>/generated

        -DCMAKE_C_COMPILER=/usr/bin/clang
        -DCMAKE_CXX_COMPILER=/usr/bin/clang++

        -DTHREADS_PREFER_PTHREAD_FLAG=ON
        -DCMAKE_THREAD_LIBS_INIT=-lpthread
        -DCMAKE_HAVE_THREADS_LIBRARY=1
        -DCMAKE_HAVE_PTHREADS_CREATE=1
  DEPENDS wasm
)

# add_dependencies(native wasm-install)

# Convenience meta-target so `cmake --build .` builds everything
add_custom_target(all_build DEPENDS native)


# ─── run_wasm_host: build-native then invoke the host ───────────────────────
add_custom_target(run_wasm_host
  COMMAND ${CMAKE_BINARY_DIR}/install/bin/wasm_host
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMENT "▶ Running wasm_host"
)






