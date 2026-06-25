#!/bin/sh

set -xe

CFlags="-Wall -Wextra -ggdb -O0 -Wno-missing-field-initializers -Wno-unused-parameter -Wno-sign-compare -Wno-unused-variable -Wno-unused-function"
SourceFiles="src/main.cpp"

# Create folder "build", if not exists. Change it later.
mkdir -p build

clang $CFlags $SourceFiles -o build/example
