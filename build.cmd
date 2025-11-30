@echo off
setlocal enabledelayedexpansion

:: pnq build script - builds x64 and ARM64 configurations
:: Usage: build.cmd [clean]
:: For testing, use test-x64.cmd or test-arm64.cmd

set "ROOT=%~dp0"
cd /d "%ROOT%"

if "%1"=="clean" goto :clean

:: ============================================================================
:: x64 Build
:: ============================================================================
echo.
echo ============================================================================
echo Building x64...
echo ============================================================================

if not exist build-x64 mkdir build-x64

cmake -B build-x64 -A x64 -DPNQ_BUILD_TESTS=ON
if errorlevel 1 (
    echo ERROR: CMake configure failed for x64
    exit /b 1
)

cmake --build build-x64 --config Debug
if errorlevel 1 (
    echo ERROR: Build failed for x64
    exit /b 1
)

echo x64 build passed!

:: ============================================================================
:: ARM64 Build
:: ============================================================================
echo.
echo ============================================================================
echo Building ARM64...
echo ============================================================================

if not exist build-arm64 mkdir build-arm64

cmake -B build-arm64 -A ARM64 -DPNQ_BUILD_TESTS=ON
if errorlevel 1 (
    echo ERROR: CMake configure failed for ARM64
    exit /b 1
)

cmake --build build-arm64 --config Debug
if errorlevel 1 (
    echo ERROR: Build failed for ARM64
    exit /b 1
)

echo ARM64 build passed!

:: ============================================================================
:: Done
:: ============================================================================
echo.
echo ============================================================================
echo All builds passed!
echo Run test-x64.cmd or test-arm64.cmd to run tests on matching hardware.
echo ============================================================================
exit /b 0

:clean
echo Cleaning build directories...
if exist build-x64 rmdir /s /q build-x64
if exist build-arm64 rmdir /s /q build-arm64
echo Clean complete.
exit /b 0
