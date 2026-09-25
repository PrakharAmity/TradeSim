#!/usr/bin/env bash
set -e
mkdir -p build
cmake -B build -DCMAKE_BUILD_TYPE=Release >&2
cmake --build build --target run_tests -j2 >&2
./build/tests/run_tests
