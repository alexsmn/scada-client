@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%\..\..\..") do set "REPO_ROOT=%%~fI"
if "%~1"=="" (
  set "DOCS_IMG_DIR=C:\tc\scada\scada-docs\img"
) else (
  set "DOCS_IMG_DIR=%~f1"
)

set "GENERATOR_EXE=%REPO_ROOT%\build\ninja-dev\bin\RelWithDebInfo\screenshot_generator.exe"
set "SCREENSHOT_SRC_DIR=%REPO_ROOT%\client\docs\screenshots"

echo Building screenshot_generator if needed
pushd "%REPO_ROOT%" >nul || exit /b 1
cmake --build --preset release-dev --target screenshot_generator || (
  popd >nul
  exit /b 1
)
popd >nul

if not exist "%GENERATOR_EXE%" (
  echo screenshot_generator.exe not found at "%GENERATOR_EXE%" 1>&2
  echo Build step completed but the executable is still missing. 1>&2
  exit /b 1
)

if not exist "%DOCS_IMG_DIR%\" (
  echo scada-docs img dir not found: "%DOCS_IMG_DIR%" 1>&2
  exit /b 1
)
if not exist "%SCREENSHOT_SRC_DIR%\" (
  echo client docs screenshots dir not found: "%SCREENSHOT_SRC_DIR%" 1>&2
  exit /b 1
)

echo Publishing supported images to "%DOCS_IMG_DIR%"
for %%F in (
  client-login.png
  client-retransmission.png
  graph-cursor.png
  users.png
) do (
  if not exist "%SCREENSHOT_SRC_DIR%\%%F" (
    echo Expected generated file missing: %%F 1>&2
    exit /b 1
  )
  copy /Y "%SCREENSHOT_SRC_DIR%\%%F" "%DOCS_IMG_DIR%\%%F" >nul || exit /b 1
)

echo Updated 4 scada-docs images:
for %%F in (
  client-login.png
  client-retransmission.png
  graph-cursor.png
  users.png
) do echo   %%F

exit /b 0
