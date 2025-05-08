# wasm_nanopb_build/CMakeLists.txt
set(NANOPB_DIR ../../../third_party/nanopb)

add_library(nanopb STATIC
    ${NANOPB_DIR}/pb_encode.c
    ${NANOPB_DIR}/pb_decode.c
    ${NANOPB_DIR}/pb_common.c
)

target_include_directories(nanopb PUBLIC ${NANOPB_DIR})
