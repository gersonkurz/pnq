@echo off
:: Run ARM64 tests (requires ARM64 hardware)

set "ROOT=%~dp0"
cd /d "%ROOT%"

if not exist build-arm64 (
    echo ERROR: build-arm64 directory not found. Run build.cmd first.
    exit /b 1
)

echo Running ARM64 tests...
ctest --test-dir build-arm64 --build-config Debug --output-on-failure
if errorlevel 1 (
    echo ERROR: ARM64 tests failed
    exit /b 1
)

echo ARM64 tests passed!
