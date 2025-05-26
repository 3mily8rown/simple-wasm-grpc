cmake_minimum_required(VERSION 3.15)
project(wasm_mod LANGUAGES C CXX)

# ---------- variables supplied by the super-build
foreach(var PROTO_DIR SCRIPT_DIR)
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "${var} not set")
    endif()
endforeach()

# # Python venv for protoc-gen-nanopb ----------
# find_package(Python3 REQUIRED COMPONENTS Interpreter)
# set(VENV "${CMAKE_BINARY_DIR}/nanopb_venv")
# add_custom_command(
#     OUTPUT  "${VENV}/.done"
#     COMMAND "${Python3_EXECUTABLE}" -m venv "${VENV}"
#     COMMAND "${VENV}/bin/pip" install --upgrade pip protobuf grpcio-tools
#     COMMAND "${CMAKE_COMMAND}" -E touch "${VENV}/.done")
# add_custom_target(nano_venv ALL DEPENDS "${VENV}/.done")

# Add static rpc stub library
add_subdirectory(rpc_stub)

# client WASM module -----------------------------------------------
add_executable(client_app  src/client_app.cpp)
target_link_libraries(client_app PRIVATE rpb_stub_wasm nanopb::nanopb)
target_link_options(client_app PRIVATE
    "-Wl,--export-all" "-Wl,--no-gc-sections" "-Wl,--allow-undefined")
set_target_properties(client_app PROPERTIES SUFFIX ".wasm")

install(TARGETS client_app DESTINATION wasm)

# server WASM module ------------------------------------------
add_executable(server_app  src/server_app.cpp)
target_link_libraries(server_app PRIVATE rpb_stub_wasm nanopb::nanopb)
target_link_options(server_app PRIVATE
    "-Wl,--export-all" "-Wl,--no-gc-sections" "-Wl,--allow-undefined")
set_target_properties(server_app PROPERTIES SUFFIX ".wasm")

install(TARGETS server_app DESTINATION wasm)

# ensure install‐tree exists
file(MAKE_DIRECTORY "${IMPORTS_DIR}")

# paths to the built .wasm files
set(CLIENT_WASM  "$<TARGET_FILE:client_app>")
set(SERVER_WASM  "$<TARGET_FILE:server_app>")

# target header paths
set(CLIENT_HDR  "${IMPORTS_DIR}/client_app_imports.h")
set(SERVER_HDR  "${IMPORTS_DIR}/server_app_imports.h")

# run import script once both .wasm exist
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

