cmake_minimum_required(VERSION 3.15)
project(wasm_mod LANGUAGES C CXX)

# ---------- variables supplied by the super-build ----------
foreach(var PROTOC_EXECUTABLE PROTO_DIR SCRIPT_DIR)
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "${var} not set")
    endif()
endforeach()

# ---------- bring in Nanopb (runtime + generator) ----------
include(FetchContent)
set(nanopb_BUILD_GENERATOR OFF  CACHE BOOL "" FORCE)
set(nanopb_BUILD_RUNTIME   ON  CACHE BOOL "" FORCE)

FetchContent_Declare(
    nanopb
    GIT_REPOSITORY https://github.com/nanopb/nanopb.git
    GIT_TAG        0.4.9
)
FetchContent_MakeAvailable(nanopb)


# Create the alias we’d like to use everywhere
add_library(nanopb::nanopb ALIAS protobuf-nanopb-static)

# ---------- Python venv for protoc-gen-nanopb ----------
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(VENV "${CMAKE_BINARY_DIR}/nanopb_venv")
add_custom_command(
    OUTPUT  "${VENV}/.done"
    COMMAND "${Python3_EXECUTABLE}" -m venv "${VENV}"
    COMMAND "${VENV}/bin/pip" install --upgrade pip protobuf grpcio-tools
    COMMAND "${CMAKE_COMMAND}" -E touch "${VENV}/.done")
add_custom_target(nano_venv ALL DEPENDS "${VENV}/.done")

# Wrapper script so protoc finds the generator in the venv
set(WRAP "${CMAKE_BINARY_DIR}/protoc-gen-nanopb")
file(WRITE  "${WRAP}"
  "#!/usr/bin/env bash\nexec \"${VENV}/bin/python\" \"${nanopb_SOURCE_DIR}/generator/nanopb_generator.py\" --protoc-plugin \"$@\"")
file(CHMOD "${WRAP}" PERMISSIONS
     OWNER_READ OWNER_WRITE OWNER_EXECUTE
     GROUP_READ GROUP_EXECUTE
     WORLD_READ WORLD_EXECUTE)

# ---------- generate message.pb.{c,h} ----------
set(GEN "${CMAKE_BINARY_DIR}/generated_nano")
file(MAKE_DIRECTORY "${GEN}")

add_custom_command(
    OUTPUT  "${GEN}/message.pb.c" "${GEN}/message.pb.h"
    WORKING_DIRECTORY "${PROTO_DIR}"          # lets us say just "message.proto"
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

add_library(proto_nano OBJECT "${GEN}/message.pb.c")
target_include_directories(proto_nano PUBLIC "${GEN}" "${nanopb_SOURCE_DIR}")
target_link_libraries(proto_nano PUBLIC nanopb::nanopb)

# ─── client WASM module ───────────────────────────────────────────────────
add_executable(client_app  src/client_app.cpp)
target_link_libraries(client_app
    PRIVATE
        proto_nano          # generated protobuf code
        nanopb::nanopb)     # Nanopb runtime
target_link_options(client_app PRIVATE
    "-Wl,--export-all" "-Wl,--no-gc-sections" "-Wl,--allow-undefined")
set_target_properties(client_app PROPERTIES SUFFIX ".wasm")

install(TARGETS client_app DESTINATION wasm)

# ─── server WASM module ───────────────────────────────────────────────────
add_executable(server_app  src/server_app.cpp)
target_link_libraries(server_app
    PRIVATE
        proto_nano
        nanopb::nanopb)
target_link_options(server_app PRIVATE
    "-Wl,--export-all" "-Wl,--no-gc-sections" "-Wl,--allow-undefined")
set_target_properties(server_app PROPERTIES SUFFIX ".wasm")

install(TARGETS server_app DESTINATION wasm)


# ---------- install for the host stage ----------
# doesnt need it
install(FILES "${GEN}/message.pb.h" DESTINATION generated_nano)

# ─── generate + install WAMR native‐stub headers ───────────────────────────
if(NOT DEFINED IMPORTS_DIR)
  message(FATAL_ERROR "IMPORTS_DIR must be set to your <install>/generated directory")
endif()

# ensure the install‐tree exists
file(MAKE_DIRECTORY "${IMPORTS_DIR}")

# compute the paths to the built .wasm files
set(CLIENT_WASM  "$<TARGET_FILE:client_app>")
set(SERVER_WASM  "$<TARGET_FILE:server_app>")

# target header paths
set(CLIENT_HDR  "${IMPORTS_DIR}/client_app_imports.h")
set(SERVER_HDR  "${IMPORTS_DIR}/server_app_imports.h")

# custom command: run your Python script once both .wasm exist
add_custom_command(
  OUTPUT  ${CLIENT_HDR} ${SERVER_HDR}
  COMMAND "${Python3_EXECUTABLE}"
          "${SCRIPT_DIR}/generate_wasm_imports.py"
            --module client_app    --native rpc/native_impl.h
            "${CLIENT_WASM}"       "${CLIENT_HDR}"
  COMMAND "${Python3_EXECUTABLE}"
          "${SCRIPT_DIR}/generate_wasm_imports.py"
            --module server_app    --native rpc/native_impl.h
            "${SERVER_WASM}"       "${SERVER_HDR}"
  DEPENDS client_app server_app
  COMMENT "⟳ Generating WAMR native import‐stub headers"
  VERBATIM
)

# make it part of the normal build
add_custom_target(generate_import_headers ALL
  DEPENDS ${CLIENT_HDR} ${SERVER_HDR}
)

# install the headers into IMPORTS_DIR on 'install'
install(FILES ${CLIENT_HDR} ${SERVER_HDR}
        DESTINATION "${IMPORTS_DIR}")

