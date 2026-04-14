@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%\..\..") do set "REPO_ROOT=%%~fI"
if "%~1"=="" (
  set "DOCS_IMG_DIR=%REPO_ROOT%\scada-docs\img"
) else (
  set "DOCS_IMG_DIR=%~f1"
)

set "GENERATOR_EXE=%REPO_ROOT%\build\ninja-dev\bin\RelWithDebInfo\screenshot_generator.exe"
set "TMP_DIR=%TEMP%\scada-docs-screenshots-%RANDOM%%RANDOM%"

if not exist "%GENERATOR_EXE%" (
  echo screenshot_generator.exe not found at "%GENERATOR_EXE%" 1>&2
  echo Build it first: cmake --build --preset release-dev --target screenshot_generator 1>&2
  exit /b 1
)

if not exist "%DOCS_IMG_DIR%\" (
  echo scada-docs img dir not found: "%DOCS_IMG_DIR%" 1>&2
  exit /b 1
)

mkdir "%TMP_DIR%" >nul || exit /b 1

echo Rendering screenshots to "%TMP_DIR%"
set "SCREENSHOT_OUT_DIR=%TMP_DIR%"
"%GENERATOR_EXE%" || goto :cleanup

echo Publishing supported images to "%DOCS_IMG_DIR%"
for %%F in (
  client-retransmission.png
  users.png
) do (
  if not exist "%TMP_DIR%\%%F" (
    echo Expected generated file missing: %%F 1>&2
    goto :cleanup_fail
  )
  copy /Y "%TMP_DIR%\%%F" "%DOCS_IMG_DIR%\%%F" >nul || goto :cleanup_fail
)

echo Updated 2 scada-docs images:
for %%F in (
  client-retransmission.png
  users.png
) do echo   %%F

rmdir /S /Q "%TMP_DIR%" >nul 2>nul
exit /b 0

:cleanup_fail
set "EXIT_CODE=1"
goto :cleanup_final

:cleanup
set "EXIT_CODE=%ERRORLEVEL%"

:cleanup_final
rmdir /S /Q "%TMP_DIR%" >nul 2>nul
exit /b %EXIT_CODE%
