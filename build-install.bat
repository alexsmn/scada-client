set BUILD_DIR=build-cmake

@if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%" || exit /b 1

@pushd "%BUILD_DIR%" || exit /b 1
cpack.exe -C $(Configuration) --config ./CPackConfig.cmake
@popd
