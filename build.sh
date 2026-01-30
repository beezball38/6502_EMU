#!/usr/bin/env bash

case "$1" in
    clean)
        rm -rf build
        echo "Clean complete"
        ;;
    test)
        mkdir -p build && cd build && cmake .. && make && cd .. && \
        echo "=== Running unit tests ===" && \
        ./build/bin/tests/emulator_tests && \
        echo "" && \
        echo "=== Running nestest (legal opcodes) ===" && \
        ./build/bin/cpu_trace --nestest -q -n 5003
        ;;
    *)
        mkdir -p build && cd build && cmake .. && make
        ;;
esac
