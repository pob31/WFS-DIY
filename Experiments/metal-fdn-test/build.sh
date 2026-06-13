#!/bin/bash
# Build the headless MetalFdnBackend correctness/timing test (no Xcode project).
cd "$(dirname "$0")"
clang++ -std=c++17 -O2 -fobjc-arc \
    -DWFS_GPU_NATIVE=1 \
    -framework Metal -framework Foundation \
    backend_test.mm ../../Source/DSP/gpu/MetalFdnBackend.mm \
    -I ../../Source/DSP/gpu \
    -o backend_test
echo "built: ./backend_test"
