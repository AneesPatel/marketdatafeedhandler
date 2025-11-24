@echo off
setlocal enabledelayedexpansion

echo === Market Data Feed Handler - Windows Build Script ===
echo.

set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release

set RUN_TESTS=%2
if "%RUN_TESTS%"=="" set RUN_TESTS=true

echo Build Type: %BUILD_TYPE%
echo Run Tests: %RUN_TESTS%
echo.

if not exist build mkdir build
cd build

echo === Configuring CMake ===
cmake -G "Visual Studio 16 2019" -A x64 ..

echo.
echo === Building Project ===
cmake --build . --config %BUILD_TYPE% -j

if "%RUN_TESTS%"=="true" (
    echo.
    echo === Running Unit Tests ===
    .\%BUILD_TYPE%\unit_tests.exe
)

echo.
echo === Build Complete ===
echo Binaries in: build\%BUILD_TYPE%\
echo   - market_feed_handler.exe: Main application
echo   - unit_tests.exe: Test suite
echo   - advanced_benchmark.exe: Performance benchmarks
echo.
echo Run with: .\%BUILD_TYPE%\market_feed_handler.exe live

cd ..
