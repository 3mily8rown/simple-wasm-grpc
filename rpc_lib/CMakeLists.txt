cmake_minimum_required(VERSION 3.15)
project(rpc_lib LANGUAGES C CXX)

# 1) Fetch nanopb runtime (no generator)
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

# 2) Build the RPC library
add_library(rpc_lib STATIC
  # your implementation
  src/rpc_client.cpp
  src/rpc_server.cpp

  # the pregenerated envelope/response stubs
  src/generated_nano/rpc_envelope.pb.c
)

# 3) Public include dirs
target_include_directories(rpc_lib
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/generated_nano>
    # Protoc plugins usually generate files to avoid needing this v
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../wasm/generated_nano>
)

# 4) Link in nanopb runtime
target_link_libraries(rpc_lib PUBLIC nanopb::nanopb)

# 5) Force WASI flags (assuming you pass in your toolchain file at configure)
target_compile_options(rpc_lib
  PUBLIC
    --target=wasm32-wasi
    -fno-exceptions
    -fno-rtti
)
set_target_properties(rpc_lib PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  PUBLIC_HEADER "include/rpc_client.h;include/rpc_server.h"
)

# 6) Install/export targets so users can find_package(rpc_lib)
include(GNUInstallDirs)
install(TARGETS rpc_lib
  EXPORT rpcLibTargets
  ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rpc_lib
)

install(EXPORT rpcLibTargets
  NAMESPACE rpc_lib::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rpc_lib
)

# Optionally provide a config file so downstream can simply do:
#   find_package(rpc_lib CONFIG REQUIRED)
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/rpc_libConfigVersion.cmake"
  VERSION 1.0.0
  COMPATIBILITY AnyNewerVersion
)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindRpcLib.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/rpc_libConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rpc_lib
)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/rpc_libConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/rpc_libConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rpc_lib
)
