cmake_minimum_required(VERSION 3.15)
project(rpc_stub LANGUAGES C CXX)

# variables supplied by super-builds ----------
foreach(var PROTOC_EXECUTABLE PROTO_DIR PIGWEED_DIR VENV)
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "${var} not set")
    endif()
endforeach()

# bring in Nanopb (runtime + generator) ----------
include(FetchContent)
set(nanopb_BUILD_GENERATOR OFF  CACHE BOOL "" FORCE)
set(nanopb_BUILD_RUNTIME   ON  CACHE BOOL "" FORCE)

FetchContent_Declare(
    nanopb
    GIT_REPOSITORY https://github.com/nanopb/nanopb.git
    GIT_TAG        0.4.9
)
FetchContent_MakeAvailable(nanopb)

# Alias nanopb
add_library(nanopb::nanopb ALIAS protobuf-nanopb-static)

# Wrapper script so protoc finds the generator in the venv
set(WRAP "${CMAKE_BINARY_DIR}/protoc-gen-nanopb")
file(WRITE  "${WRAP}"
  "#!/usr/bin/env bash\nexec \"${VENV}/bin/python\" \"${nanopb_SOURCE_DIR}/generator/nanopb_generator.py\" --protoc-plugin \"$@\"")
file(CHMOD "${WRAP}" PERMISSIONS
     OWNER_READ OWNER_WRITE OWNER_EXECUTE
     GROUP_READ GROUP_EXECUTE
     WORLD_READ WORLD_EXECUTE)

# generate message.pb.{c,h} ----------
set(GEN "${CMAKE_BINARY_DIR}/generated_nano")
file(MAKE_DIRECTORY "${GEN}")

add_custom_command(
    OUTPUT  "${GEN}/message.pb.c" "${GEN}/message.pb.h"
    WORKING_DIRECTORY "${PROTO_DIR}" 
    COMMAND "${PROTOC_EXECUTABLE}"
            --plugin=protoc-gen-nanopb=${WRAP}
            --proto_path="${PROTO_DIR}"
            --proto_path="${nanopb_SOURCE_DIR}/generator/proto"
            --nanopb_out="${GEN}"
            "message.proto"
    DEPENDS
        "${PROTO_DIR}/message.proto"
        "${PROTO_DIR}/message.options"
        nano_venv)

# add_custom_target(gen_nano DEPENDS "${GEN}/message.pb.c" "${GEN}/message.pb.h")

# wrapper script so protoc finds the Pigweed RPC plugin
set(PWPB_WRAPPER "${CMAKE_BINARY_DIR}/protoc-gen-pwpb_rpc")
file(WRITE "${PWPB_WRAPPER}"
  "#!/usr/bin/env bash\n"
  "export PYTHONPATH=\"${PIGWEED_DIR}/pw_protobuf_compiler/py:${PIGWEED_DIR}:\$PYTHONPATH\"\n"
  "exec \"${Python3_EXECUTABLE}\" -m pw_protobuf_compiler.protoc_gen_pwpb_rpc \"$@\"\n")
file(CHMOD "${PWPB_WRAPPER}" PERMISSIONS
  OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_EXECUTE
  WORLD_READ WORLD_EXECUTE)





# # Generate nanopb RPC stubs using Pigweed
# set(RPC_GEN "${CMAKE_BINARY_DIR}/generated_rpc_nano")
# file(MAKE_DIRECTORY "${RPC_GEN}")

# add_custom_command(
#   OUTPUT "${RPC_GEN}/message.rpc.pb.c" "${RPC_GEN}/message.rpc.pb.h"
#   WORKING_DIRECTORY "${PROTO_DIR}"
#   COMMAND "${PROTOC_EXECUTABLE}"
#           --plugin=protoc-gen-pw_rpc=${PIGWEED_PLUGIN}
#           --proto_path="${PROTO_DIR}"
#           --proto_path="${PIGWEED_DIR}/pw_rpc"
#           --proto_path="${PIGWEED_DIR}/pw_protobuf"
#           --pw_rpc_out="${RPC_GEN}"
#           "message.proto"
#   DEPENDS "${PROTO_DIR}/message.proto"
# )

add_library(proto_nano STATIC "${GEN}/message.pb.c")
# add_dependencies(proto_nano gen_nano)
target_include_directories(proto_nano PUBLIC "${GEN}" "${nanopb_SOURCE_DIR}")
target_link_libraries(proto_nano PUBLIC nanopb::nanopb)

# add_library(rpc_stub_wasm STATIC "${RPC_GEN}/message.rpc.pb.c")

# target_include_directories(rpc_stub_wasm PUBLIC
#     "${RPC_GEN}"
#     "${GEN}"
#     "${nanopb_SOURCE_DIR}"
#     "${PIGWEED_DIR}/pw_rpc/public")
# target_link_libraries(rpc_stub_wasm PUBLIC nanopb::nanopb proto_nano)

# add_library(rpc_stub_wasm ALIAS rpc_stub_wasm)
# ----------------------------------------------------------------------

# ── E.  Generate the **Pigweed RPC** stub header ────────────────────
set(RPC_GEN "${CMAKE_BINARY_DIR}/generated_rpc")
file(MAKE_DIRECTORY "${RPC_GEN}")

# 1) wrapper so protoc can find the pw_protobuf_compiler plugin in your checkout
set(PWPB_WRAPPER "${CMAKE_BINARY_DIR}/protoc-gen-pwpb_rpc")
file(WRITE "${PWPB_WRAPPER}"
  "#!/usr/bin/env bash\n"
  "export PYTHONPATH=\"${PIGWEED_DIR}/pw_protobuf_compiler/py:\${PIGWEED_DIR}:\$PYTHONPATH\"\n"
  "exec \"${Python3_EXECUTABLE}\" -m pw_protobuf_compiler.protoc_gen_pwpb_rpc \"$@\"\n")
file(CHMOD "${PWPB_WRAPPER}" PERMISSIONS
  OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_EXECUTE
  WORLD_READ WORLD_EXECUTE)

# 2) run protoc to generate ONLY the .rpc.pb.h
add_custom_command(
    OUTPUT "${RPC_GEN}/message.rpc.pb.h"
    WORKING_DIRECTORY "${PROTO_DIR}"
    COMMAND "${PROTOC_EXECUTABLE}"
        --plugin=protoc-gen-pw_rpc=${PWPB_WRAPPER}
        --proto_path="${PROTO_DIR}"
        --proto_path=${nanopb_SOURCE_DIR}/generator/proto
        --pw_rpc_out="${RPC_GEN}"
        "message.proto"
    DEPENDS "${PROTO_DIR}/message.proto" nano_venv
)


add_custom_target(stub_gen DEPENDS "${RPC_GEN}/message.rpc.pb.h")

add_library(rpc_stub_wasm STATIC
    "${GEN}/message.pb.c"
    "${RPC_GEN}/message.rpc.pb.c"
    src/wasm_transport.cpp)

target_include_directories(rpc_stub_wasm PUBLIC
    "${GEN}"
    "${RPC_GEN}"
    "${nanopb_SOURCE_DIR}"
    "${PROTO_DIR}"
)

target_link_libraries(rpc_stub_wasm PUBLIC nanopb::nanopb)
add_dependencies(rpc_stub_wasm stub_gen)


# ── G.  Finally bundle into rpc_stub_wasm ────────────────────────────

# add_library(rpc_stub_wasm STATIC
#     $<TARGET_OBJECTS:pw_rpc_client>
#     $<TARGET_OBJECTS:rpc_transport>)
# target_include_directories(rpc_stub_wasm PUBLIC
#     "${GEN}" "${RPC_GEN}" "${nanopb_SOURCE_DIR}" "${PIGWEED_DIR}")
# target_link_libraries(rpc_stub_wasm PUBLIC nanopb::nanopb proto_nano)
# add_dependencies(rpc_stub_wasm stub_gen proto_nano)

# # # Libraries --------------
# add_library(proto_nano STATIC "${GEN}/message.pb.c")
# target_include_directories(proto_nano PUBLIC "${GEN}" "${nanopb_SOURCE_DIR}")
# target_link_libraries(proto_nano PUBLIC nanopb::nanopb)

# add_library(rpb_stub_wasm STATIC $<TARGET_OBJECTS:proto_nano>)
# target_link_libraries(rpb_stub_wasm PUBLIC nanopb::nanopb proto_nano)
# add_library(rpb_stub_wasm ALIAS rpb_stub_wasm)
