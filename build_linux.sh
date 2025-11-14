#!/bin/bash

# For the first time
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/home/ankalot/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release

# rebuild
#cmake --build build --target clean --config Release
#cmake --build build --config Release