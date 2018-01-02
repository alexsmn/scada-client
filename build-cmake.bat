set BUILD_DIR=build-cmake
set THIRD_PARTY=d:/ThirdParty
set CMAKE_PREFIX_PATH=%THIRD_PARTY%/boost-1.65.1

@if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%" || exit /b 1

@pushd "%BUILD_DIR%" || exit /b 1
cmake .. -T "v141_xp" "-DCMAKE_PREFIX_PATH=%CMAKE_PREFIX_PATH%" "-Ddeps=%third_party%"
@popd
