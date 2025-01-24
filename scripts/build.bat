@echo off
set BUILD_DIR=build

if not exist %BUILD_DIR% (
    mkdir %BUILD_DIR%
)

cmake -B %BUILD_DIR% -S .
cmake --build build
