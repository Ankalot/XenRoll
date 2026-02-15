:: For the first time
::cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=E:/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Debug
::cmake --build build --config Debug

:: rebuild
del "build\plugin\XenRoll_artefacts\Debug\VST3\XenRoll.vst3\Contents\Resources\moduleinfo.json"
cmake --build build --target clean --config Debug
cmake --build build --config Debug