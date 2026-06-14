#!/bin/bash
# Build the headless Metal WFS kernel spike (no Xcode project needed).
cd "$(dirname "$0")"
clang++ -std=c++17 -O2 -fobjc-arc \
    -framework Metal -framework Foundation \
    spike.mm -o spike
echo "built: ./spike (run from this directory - it loads wfs_delay_sum.metal at runtime)"
