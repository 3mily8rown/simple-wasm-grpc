cmake_minimum_required(VERSION 3.15)
project(NativeRPCExample C CXX)
set(CMAKE_CXX_STANDARD 17)

# ——— Nanopb as before ———
include(FetchContent)
set(nanopb_BUILD_GENERATOR OFF CACHE BOOL “” FORCE)
set(nanopb_BUILD_RUNTIME   ON  CACHE BOOL “” FORCE)
FetchContent_Declare(
  nanopb
  GIT_REPOSITORY https://github.com/nanopb/nanopb.git
  GIT_TAG        0.4.9
)
FetchContent_MakeAvailable(nanopb)
add_library(nanopb::nanopb ALIAS protobuf-nanopb-static)

# ——— Proto library ———
set(GENERATED_PB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/wasm-prefix/src/wasm-build/generated_nano")
set(GENERATED_ENVELOPE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../rpc_lib/src/generated_nano")
set(NANOPB_SOURCE_DIR "${nanopb_SOURCE_DIR}")
set(NANOPB_SOURCES
  ${NANOPB_SOURCE_DIR}/pb_encode.c
  ${NANOPB_SOURCE_DIR}/pb_decode.c
  ${NANOPB_SOURCE_DIR}/pb_common.c
)
set(PROTO_SOURCES
  "${GENERATED_PB_DIR}/message.pb.c"
  "${GENERATED_ENVELOPE_DIR}/rpc_envelope.pb.c"
)
add_library(proto_nano STATIC
  ${PROTO_SOURCES}
  ${NANOPB_SOURCES}
)
target_include_directories(proto_nano PUBLIC
  ${GENERATED_PB_DIR}
  ${GENERATED_ENVELOPE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(proto_nano PUBLIC nanopb::nanopb)

# ——— Shared RPC primitives (client, server, socket, queue) ———
file(GLOB RPC_PRIMITIVES_SRCS
  ../native/src/rpc/socket_communication.cpp
  ../native/src/rpc/message_queue.cpp
  ../native/src/ring_buffer_rpc/rb_message_queue.cpp
  src/rpc_defs/rpc_client.cpp       # <-- make sure this is the correct path
  src/rpc_defs/rpc_server.cpp       # <-- if you have a server impl here too
)
add_library(rpc_primitives STATIC ${RPC_PRIMITIVES_SRCS})
target_include_directories(rpc_primitives PUBLIC
  ../native/include
  src/rpc_defs
)

# ——— Native client_app ———
add_executable(native_client src/client_app.cpp)
target_link_libraries(native_client PRIVATE proto_nano)

# ——— Native server_app ———
add_executable(native_server src/server_app.cpp)
target_link_libraries(native_server PRIVATE proto_nano)

# ——— “Same RPC” client ———
add_executable(native_client_same src/client.cpp)
target_link_libraries(native_client_same PRIVATE
  proto_nano
  rpc_primitives
)

# ——— “Same RPC” server ———
add_executable(native_server_same src/server.cpp)
target_link_libraries(native_server_same PRIVATE
  proto_nano
  rpc_primitives
)
