set BUILD_DIR=build-cmake
set THIRD_PARTY=c:/work/third_party
set CMAKE_PREFIX_PATH=%THIRD_PARTY%/boost-1.63.0;c:/Qt/5.7/msvc2015/lib/cmake

@if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%" || exit /b 1

@pushd "%BUILD_DIR%" || exit /b 1
cmake .. -T "v140_xp" "-DCMAKE_PREFIX_PATH=%CMAKE_PREFIX_PATH%" "-Ddeps=%third_party%"
@popd
