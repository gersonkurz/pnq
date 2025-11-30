@echo off
:: Run x64 tests (requires x64 hardware or emulation)

set "ROOT=%~dp0"
cd /d "%ROOT%"

if not exist build-x64 (
    echo ERROR: build-x64 directory not found. Run build.cmd first.
    exit /b 1
)

echo Running x64 tests...
ctest --test-dir build-x64 --build-config Debug --output-on-failure
if errorlevel 1 (
    echo ERROR: x64 tests failed
    exit /b 1
)

echo x64 tests passed!
