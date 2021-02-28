#! /bin/bash
mkdir -p build
pushd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../NintendoSwitchToolchain.cmake -DCMAKE_INSTALL_PREFIX=blobby -DSWITCH=true -DCMAKE_BUILD_TYPE=Debug
popd