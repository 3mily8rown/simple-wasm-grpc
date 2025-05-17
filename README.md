# simple-wasm-rpc

## Function 
Wasm client can send a message through a native backend to be received by a server

## To configure/build/run
(examples based on CMakePresets.json)
* cmake --preset default 
for when changed the toolchain or CMakeLists.txt or presets

* cmake --build --preset clean-build
reconfigures and full build

* cmake --build --preset build-native
for updates to the native code

* cmake --build --preset dev
for updates to cpp or wasm code

* cmake --build --preset dev-run
for updates to cpp or wasm code then wasm_host is ran post build

* cmake --build --preset run
build-native then run wasm_host

## Note
Initial code brought across code from experimental repository: https://github.com/3mily8rown/fyp 
