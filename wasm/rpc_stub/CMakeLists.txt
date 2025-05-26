cmake_minimum_required(VERSION 3.15)
project(rpc_stub LANGUAGES C CXX)

# variables supplied by super-builds ----------
foreach(var PROTOC_EXECUTABLE PROTO_DIR PIGWEED_DIR)
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

# Create the alias
add_library(nanopb::nanopb ALIAS protobuf-nanopb-static)

# --------------------------------------
set(PREGENERATED_DIR "${CMAKE_SOURCE_DIR}/generated_nano")
set(GEN "${CMAKE_BINARY_DIR}/generated_nano")
file(MAKE_DIRECTORY "${GEN}")

set(GENERATED_FILES
    message.pb.c
    message.pb.h
    rpc_envelope.pb.c
    rpc_envelope.pb.h
)

foreach(file ${GENERATED_FILES})
    add_custom_command(
        OUTPUT "${GEN}/${file}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${PREGENERATED_DIR}/${file}"
                "${GEN}/${file}"
        DEPENDS "${PREGENERATED_DIR}/${file}"
        COMMENT "Copying pregenerated protobuf file ${file}"
        VERBATIM
    )
endforeach()

# Define a custom target to copy all pregenerated files
add_custom_target(copy_protos ALL DEPENDS ${GEN}/message.pb.c ${GEN}/message.pb.h ${GEN}/rpc_envelope.pb.c ${GEN}/rpc_envelope.pb.h)

add_library(proto_nano STATIC
    ${GEN}/message.pb.c
    ${GEN}/rpc_envelope.pb.c
)

add_dependencies(proto_nano copy_protos)

target_include_directories(proto_nano PUBLIC "${GEN}" "${nanopb_SOURCE_DIR}")
target_link_libraries(proto_nano PUBLIC nanopb::nanopb)

add_library(rpb_stub_wasm STATIC $<TARGET_OBJECTS:proto_nano>)
target_link_libraries(rpb_stub_wasm PUBLIC nanopb::nanopb proto_nano)
add_library(rpb_stub_wasm ALIAS rpb_stub_wasm)