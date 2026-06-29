@echo off
setlocal

REM Use UTF-8 in console.
chcp 65001 > nul

set CFlags=-Wall -Wextra -ggdb -O0 ^
-Wno-missing-field-initializers ^
-Wno-unused-parameter ^
-Wno-sign-compare ^
-Wno-unused-variable ^
-Wno-unused-function

set SourceFiles=src\main.cpp

REM Create folder "build", if not exists.
if not exist build (
    mkdir build
)

clang %CFlags% %SourceFiles% -o build\example.exe

if errorlevel 1 (
    exit /b %errorlevel%
)

endlocal