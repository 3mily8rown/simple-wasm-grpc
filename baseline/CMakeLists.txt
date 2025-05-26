cmake_minimum_required(VERSION 3.15)
project(NativeRPCExample C CXX)

set(CMAKE_CXX_STANDARD 17)

# Bring in Nanopb runtime only
include(FetchContent)
set(nanopb_BUILD_GENERATOR OFF CACHE BOOL "" FORCE)
set(nanopb_BUILD_RUNTIME   ON  CACHE BOOL "" FORCE)

FetchContent_Declare(
    nanopb
    GIT_REPOSITORY https://github.com/nanopb/nanopb.git
    GIT_TAG        0.4.9
)
FetchContent_MakeAvailable(nanopb)

add_library(nanopb::nanopb ALIAS protobuf-nanopb-static)

# Path to generated Nanopb files
set(GENERATED_PB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/wasm-prefix/src/wasm-build/generated_nano")

set(NANOPB_SOURCE_DIR "${nanopb_SOURCE_DIR}")  # from FetchContent_MakeAvailable

set(NANOPB_SOURCES
    ${NANOPB_SOURCE_DIR}/pb_encode.c
    ${NANOPB_SOURCE_DIR}/pb_decode.c
    ${NANOPB_SOURCE_DIR}/pb_common.c
)

# Pre-generated .pb files (in same dir as headers)
set(PROTO_SOURCES
    "${GENERATED_PB_DIR}/message.pb.c"
    "${GENERATED_PB_DIR}/rpc_envelope.pb.c"
)

# Create proto + nanopb static library
add_library(proto_nano STATIC
    ${PROTO_SOURCES}
    ${NANOPB_SOURCES}
)
target_include_directories(proto_nano PUBLIC
    ${GENERATED_PB_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(proto_nano PUBLIC nanopb::nanopb)

# Native client
add_executable(native_client src/client_app.cpp)
target_link_libraries(native_client PRIVATE proto_nano)

# Native server
add_executable(native_server src/server_app.cpp)
target_link_libraries(native_server PRIVATE proto_nano)
