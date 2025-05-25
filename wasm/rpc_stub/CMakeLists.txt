cmake_minimum_required(VERSION 3.15)
project(rpc_stub LANGUAGES C CXX)

# these variables come from the super-build / wasm root
foreach(var PROTOC_EXECUTABLE PROTO_DIR VENV PIGWEED_DIR)
  if(NOT DEFINED ${var})
    message(FATAL_ERROR "${var} not set")
  endif()
endforeach()

# ─── 1) Fetch only the Nanopb *runtime* ─────────────────────────────────
include(FetchContent)
set(nanopb_BUILD_GENERATOR OFF CACHE BOOL "" FORCE)
set(nanopb_BUILD_RUNTIME   ON  CACHE BOOL "" FORCE)
FetchContent_Declare(
  nanopb
  GIT_REPOSITORY https://github.com/nanopb/nanopb.git
  GIT_TAG        0.4.9
)
FetchContent_MakeAvailable(nanopb)

# make an ALIAS so the top‐level can just link nanopb::nanopb
add_library(nanopb::nanopb ALIAS protobuf-nanopb-static)


# ─── 2) Write the two tiny protoc‐plugin wrappers ───────────────────────
# nanopb plugin
set(NANOPB_PLUGIN "${CMAKE_BINARY_DIR}/protoc-gen-nanopb")
file(WRITE "${NANOPB_PLUGIN}"
  "#!/usr/bin/env bash\n"
  "exec \"${VENV}/bin/python\" \"${nanopb_SOURCE_DIR}/generator/nanopb_generator.py\" --protoc-plugin \"\$@\"\n"
)
file(CHMOD "${NANOPB_PLUGIN}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)

# pw_rpc→nanopb plugin
set(NANOPB_RPC_PLUGIN "${CMAKE_BINARY_DIR}/protoc-gen-nanopb_rpc")
file(WRITE "${NANOPB_RPC_PLUGIN}"
  "#!/usr/bin/env bash\n"
  "export PYTHONPATH=\"${PIGWEED_DIR}/pw_protobuf_compiler/py:\$PYTHONPATH\"\n"
  "exec \"${VENV}/bin/python\" -m pw_protobuf_compiler.protoc_gen_pwpb_rpc \"\$@\"\n"
)
file(CHMOD "${NANOPB_RPC_PLUGIN}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)


# ─── 3) Generate both nanopb and nanopb‐RPC code ────────────────────────
set(GEN_PB  "${CMAKE_BINARY_DIR}/generated_nano")
set(GEN_RPC "${CMAKE_BINARY_DIR}/generated_rpc")
file(MAKE_DIRECTORY "${GEN_PB}" "${GEN_RPC}")

# 3a) nanopb encode/decode
add_custom_command(
  OUTPUT "${GEN_PB}/message.pb.c" "${GEN_PB}/message.pb.h"
  COMMAND "${PROTOC_EXECUTABLE}"
          --plugin=protoc-gen-nanopb=${NANOPB_PLUGIN}
          --nanopb_out=${GEN_PB}
          --proto_path=${PROTO_DIR}
          --proto_path=${nanopb_SOURCE_DIR}/generator/proto
          message.proto
  DEPENDS "${PROTO_DIR}/message.proto" nano_venv
)

# 3b) pw_rpc→nanopb stubs (.rpc.pb.c/.h)
add_custom_command(
  OUTPUT "${GEN_RPC}/message.rpc.pb.c" "${GEN_RPC}/message.rpc.pb.h"
  COMMAND "${PROTOC_EXECUTABLE}"
          --plugin=protoc-gen-nanopb_rpc=${NANOPB_RPC_PLUGIN}
          --nanopb_rpc_out=${GEN_RPC}
          --proto_path=${PROTO_DIR}
          message.proto
  DEPENDS "${PROTO_DIR}/message.proto" nano_venv
)

# group them into one target so they always run before we build
add_custom_target(gen_protos ALL
  DEPENDS
    "${GEN_PB}/message.pb.c"
    "${GEN_RPC}/message.rpc.pb.c"
)


# ─── 4) Wrap the generated C-sources as ordinary STATIC libs ────────────
add_library(proto_nano STATIC "${GEN_PB}/message.pb.c")
target_include_directories(proto_nano PUBLIC "${GEN_PB}")
add_dependencies(proto_nano gen_protos)

add_library(rpb_stub_wasm STATIC "${GEN_RPC}/message.rpc.pb.c")
target_include_directories(rpb_stub_wasm PUBLIC
  "${GEN_RPC}"
  "${PIGWEED_DIR}/pw_rpc/public"         # for pw_rpc/channel.h etc.
  "${PIGWEED_DIR}/pw_bytes/public"       # for span.h
  "${PIGWEED_DIR}/pw_span/public"
  "${PIGWEED_DIR}/pw_status/public"
  "${PIGWEED_DIR}/pw_assert/public"
  "${PIGWEED_DIR}/pw_assert_backend/public"
  "${PIGWEED_DIR}/pw_log/basic/public"
)
add_dependencies(rpb_stub_wasm gen_protos)

# make proto_nano and rpb_stub_wasm link against the Nanopb runtime
target_link_libraries(rpb_stub_wasm PUBLIC nanopb::nanopb proto_nano)


# ─── Done! ──────────────────────────────────────────────────────────────
# Now your top‐level wasm/CMakeLists.txt just does:
#
#   add_subdirectory(rpc_stub)
#   target_link_libraries(client_app PRIVATE
#     rpb_stub_wasm
#     nanopb::nanopb
#     proto_nano)
#
# and everything (venv, protoc, fetch nanopb, build stubs) will
# automatically happen in the right order.
