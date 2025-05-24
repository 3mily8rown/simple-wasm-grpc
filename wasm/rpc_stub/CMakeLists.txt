cmake_minimum_required(VERSION 3.15)
project(rpc_stub LANGUAGES C CXX)

# variables supplied by super-builds ----------
foreach(var PROTOC_EXECUTABLE PROTO_DIR PIGWEED_DIR)
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "${var} not set")
    endif()
endforeach()

# TODO For some reason super build fails if this is here and not in the wasm CMakeLists.txt
# # Python venv for protoc-gen-nanopb ----------
# find_package(Python3 REQUIRED COMPONENTS Interpreter)
# set(VENV "${CMAKE_BINARY_DIR}/nanopb_venv")
# add_custom_command(
#     OUTPUT  "${VENV}/.done"
#     COMMAND "${Python3_EXECUTABLE}" -m venv "${VENV}"
#     COMMAND "${VENV}/bin/pip" install --upgrade pip protobuf grpcio-tools
#     COMMAND "${CMAKE_COMMAND}" -E touch "${VENV}/.done")
# add_custom_target(nano_venv ALL DEPENDS "${VENV}/.done")

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

# Create the alias
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

add_library(proto_nano STATIC "${GEN}/message.pb.c")
target_include_directories(proto_nano PUBLIC "${GEN}" "${nanopb_SOURCE_DIR}")
target_link_libraries(proto_nano PUBLIC nanopb::nanopb)

add_library(rpb_stub_wasm STATIC $<TARGET_OBJECTS:proto_nano>)
target_link_libraries(rpb_stub_wasm PUBLIC nanopb::nanopb proto_nano)
add_library(rpb_stub_wasm ALIAS rpb_stub_wasm)
