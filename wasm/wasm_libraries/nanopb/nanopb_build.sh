PROJECT_ROOT=../../..
NANOPB="$PROJECT_ROOT"/third_party/nanopb

[ -d build ] && rm -rf build
mkdir -p build && cd build

cmake .. \
  -DNANOPB_DIR=${NANOPB_DIR} \
  -DCMAKE_TOOLCHAIN_FILE=../../wasi-toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release

make -j$(nproc)