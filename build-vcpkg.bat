set BUILD_DIR=build-vcpkg
set THIRD_PARTY=d:\ThirdParty

@if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%" || exit /b 1

@pushd "%BUILD_DIR%" || exit /b 1
cmake .. -T "v141_xp" ^
  "-DCMAKE_TOOLCHAIN_FILE:PATH=d:\vcpkg\scripts\buildsystems\vcpkg.cmake" ^
  "-DCMAKE_MODULE_PATH:PATH=%THIRD_PARTY%\skia" ^
  "-Ddeps:PATH=%THIRD_PARTY%"
@popd
