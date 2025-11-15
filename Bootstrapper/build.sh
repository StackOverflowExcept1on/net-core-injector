#!/usr/bin/env bash
set -ex

mkdir -p build && cd build

cmake -DCMAKE_BUILD_TYPE=Release ..

cmake --build .
cmake --install .
