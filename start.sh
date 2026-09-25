#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"
mkdir -p build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target tradesim -j2
PORT=8080 exec ./build/tradesim